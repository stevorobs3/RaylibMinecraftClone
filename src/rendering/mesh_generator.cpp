#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>
#include <type_traits>

#include <array>

#include "mesh_generator.h"
#include "quad.h"
#include "world/block.h"
#include "world/chunk_section.h"

namespace {

// todo: also will need to be aware of block shape (half block, stairs, etc)
// todo: quesion: can this be reused for things like chests? or would that need to
// be separate?
std::vector<Quad> quads_from(
  const ChunkSection& chunk_section
) {
  std::vector<Quad> quads;

  auto blocks = chunk_section.const_blocks();
  // internal blocks can use the efficient lookup of blocks
  // todo: maybe have an iterator in the chunk_section for:
  // 1) internal blocks
  // 2) edge / perimiter / surface blocks (better name needed)
  for (int x = 1; x < ChunkSection::n - 1; x++) {
    for (int y = 1; y < ChunkSection::n - 1; y++) {
      for (int z = 1; z < ChunkSection::n - 1; z++) {
        if (blocks[x,y,z] == BlockType::AIR) continue;
        for (Direction dir : Quad::directions) {
          auto neighbour = chunk_section.internal_neighbour(dir,{x,y,z});
          //todo: deeper checks here for different block shapes
          if (neighbour == BlockType::AIR) {
            quads.push_back(Quad{blocks[x,y,z], dir, {x,y,z}});
          }
        }
      }
    }
  }

  // external blocks need to use world to check neighbouring chunks

  //todo: pull this into a reusable thing with std::views::iota for x,y and z
  // top and bottom faces
  for (int y : {0, ChunkSection::n - 1}) {
    for (int x = 0; x < ChunkSection::n; x++) {
      for (int z = 0; z < ChunkSection::n; z++) {
        auto block_position = Vector3Int{
          x + static_cast<int>(chunk_section.origin.x),
          y + static_cast<int>(chunk_section.origin.y),
          z + static_cast<int>(chunk_section.origin.z)
        };
        if (chunk_section.world.get(block_position) == BlockType::AIR) continue;
        for (Direction dir : Quad::directions) {
          auto neighbour_pos = block_position + Quad::lookup_normal(dir);
          // todo: remove this once world is infinitely generated
          if (!chunk_section.world.contains(neighbour_pos) || chunk_section.world.get(neighbour_pos) == BlockType::AIR) {
            quads.push_back(Quad{
              chunk_section.world.get(block_position),
              dir,
              {x,y,z}
            });
          }
        }
      }
    }
  }

  // east and west faces
  for (int x : {0, ChunkSection::n - 1}) {
    // we've already done y = 0 and y = ChunkSection::n - 1 for top and bottom faces
    for (int y = 1; y < ChunkSection::n - 1; y++) {
      for (int z = 0; z < ChunkSection::n; z++) {
        auto block_position = Vector3Int{
          x + static_cast<int>(chunk_section.origin.x),
          y + static_cast<int>(chunk_section.origin.y),
          z + static_cast<int>(chunk_section.origin.z)
        };
        if (chunk_section.world.get(block_position) == BlockType::AIR) continue;
        for (Direction dir : Quad::directions) {
          auto neighbour_pos = block_position + Quad::lookup_normal(dir);
          // todo: remove this once world is infinitely generated
          if (!chunk_section.world.contains(neighbour_pos) || chunk_section.world.get(neighbour_pos) == BlockType::AIR) {
            quads.push_back(Quad{
              chunk_section.world.get(block_position),
              dir,
              {x,y,z}
            });
          }
        }
      }
    }
  }


  // north and south faces
  for (int z : {0, ChunkSection::n - 1}) {
    // we've already done x = 0 and ChunkSection::n - 1 for east and west faces
    for (int x = 1; x < ChunkSection::n - 1; x++) {
      // we've already done y = 0 and y = ChunkSection::n - 1 for top and bottom faces
      for (int y = 1; y < ChunkSection::n - 1; y++) {
        auto block_position = Vector3Int{
          x + static_cast<int>(chunk_section.origin.x),
          y + static_cast<int>(chunk_section.origin.y),
          z + static_cast<int>(chunk_section.origin.z)
        };
        if (chunk_section.world.get(block_position) == BlockType::AIR) continue;
        for (Direction dir : Quad::directions) {
          auto neighbour_pos = block_position + Quad::lookup_normal(dir);
          // todo: remove this once world is infinitely generated
          if (!chunk_section.world.contains(neighbour_pos) || chunk_section.world.get(neighbour_pos) == BlockType::AIR) {
            quads.push_back(Quad{
              chunk_section.world.get(block_position),
              dir,
              {x,y,z}}
            );
          }
        }
      }
    }
  }

  return quads;
}

}


std::optional<MeshData> MeshGenerator::build(const ChunkSection &chunk_section) const {

    // step 1:
    // todo: quads!
    // build up a conceptual list of faces to render
    auto quads_to_render = quads_from(chunk_section);

    if (quads_to_render.empty()) {
      TraceLog(LOG_INFO, std::format(
        "No mesh to render for:{}",
        to_string(chunk_section.origin)
      ).c_str());
      return std::nullopt;
    } else {
      TraceLog(LOG_INFO, std::format(
        "faces_to_render:{} for {}",
        quads_to_render.size(),
        to_string(chunk_section.origin)
      ).c_str());
    }

    if (quads_to_render.size() * 4 > 65536u)
      throw std::length_error("raylib Mesh uses 16-bit indices");

    MeshData data;
    data.vertex_count = static_cast<int>(quads_to_render.size() * 4);
    data.triangle_count = static_cast<int>(quads_to_render.size() * 2);
    data.vertices.reserve(data.vertex_count * 3);
    data.normals.reserve(data.vertex_count * 3);
    data.texcoords.reserve(data.vertex_count * 2);
    data.colors.reserve(data.vertex_count * 4);
    data.indices.reserve(quads_to_render.size() * 6);
    for (const auto& quad : quads_to_render) {
      const auto tex_coords = texture_atlas.lookup_texture_uv(quad.block_type, quad.direction);
      unsigned short start = static_cast<unsigned short>(data.vertices.size() / 3);
      for (int i = 0; i < Quad::n_vertices; ++i) {
        auto v = quad.vertices[i];
        data.vertices.insert(data.vertices.end(), {v.x, v.y, v.z});
        data.normals.insert(data.normals.end(), {quad.normal.x,quad.normal.y,quad.normal.z});
        data.texcoords.insert(data.texcoords.end(), {tex_coords[i].x,tex_coords[i].y});
        data.colors.insert(data.colors.end(), {quad.color.r,quad.color.g,quad.color.b,quad.color.a});
      }
      data.indices.insert(data.indices.end(), {
        static_cast<unsigned short>(start), static_cast<unsigned short>(start+2),
        static_cast<unsigned short>(start+1), static_cast<unsigned short>(start+3),
        static_cast<unsigned short>(start+1), static_cast<unsigned short>(start+2)
      });
    }
    return data;
}

Mesh MeshGenerator::upload(const MeshData& data) {
  Mesh mesh{};
  mesh.vertexCount = data.vertex_count;
  mesh.triangleCount = data.triangle_count;
  auto copy = [](const auto& src) {
    using T = typename std::decay_t<decltype(src)>::value_type;
    T* ptr = static_cast<T*>(MemAlloc(src.size() * sizeof(T)));
    if (!ptr) throw std::bad_alloc();
    std::memcpy(ptr, src.data(), src.size() * sizeof(T));
    return ptr;
  };
  try {
    mesh.vertices = copy(data.vertices);
    mesh.texcoords = copy(data.texcoords);
    mesh.normals = copy(data.normals);
    mesh.colors = copy(data.colors);
    mesh.indices = copy(data.indices);
  } catch (...) {
    MemFree(mesh.vertices);
    MemFree(mesh.texcoords);
    MemFree(mesh.normals);
    MemFree(mesh.colors);
    MemFree(mesh.indices);
    throw;
  }
  UploadMesh(&mesh, false);
  return mesh;
}

std::optional<Mesh> MeshGenerator::from(ChunkSection& section) const {
  auto data = build(section);
  return data ? std::optional<Mesh>(upload(*data)) : std::nullopt;
}
