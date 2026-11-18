
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
        auto block_position = Vector3Int(x,y,z) + chunk_section.origin;
        if (chunk_section.world.get(block_position) == BlockType::AIR) continue;
        for (Direction dir : Quad::directions) {
          auto neighbour_pos = block_position + Quad::lookup_normal(dir);
          // todo: remove this once world is infinitely generated
          if (!chunk_section.world.contains(neighbour_pos))
            continue;
          BlockType neighbour = chunk_section.world.get(neighbour_pos);
          if (neighbour == BlockType::AIR) {
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
        auto block_position = Vector3Int(x,y,z) + chunk_section.origin;
        if (chunk_section.world.get(block_position) == BlockType::AIR) continue;
        for (Direction dir : Quad::directions) {
          auto neighbour_pos = block_position + Quad::lookup_normal(dir);
          // todo: remove this once world is infinitely generated
          if (!chunk_section.world.contains(neighbour_pos))
            continue;
          auto neighbour = chunk_section.world.get(neighbour_pos);
          if (neighbour == BlockType::AIR) {
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
        auto block_position = Vector3Int(x,y,z) + chunk_section.origin;
        if (chunk_section.world.get(block_position) == BlockType::AIR) continue;
        for (Direction dir : Quad::directions) {
          auto neighbour_pos = block_position + Quad::lookup_normal(dir);
          // todo: remove this once world is infinitely generated
          if (!chunk_section.world.contains(neighbour_pos))
            continue;
          auto neighbour = chunk_section.world.get(neighbour_pos);
          if (neighbour == BlockType::AIR) {
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


std::optional<Mesh> MeshGenerator::from(ChunkSection &chunk_section) {

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

    // step 2:
    // iterate over that list, generating all the mesh vertices

    int vertex_index = 0;
    int vertex_count = 0;
    int normal_index = 0;
    int tex_coords_index = 0;
    int indices_index = 0;
    int colours_index = 0;

    Mesh mesh{};

    mesh.triangleCount = quads_to_render.size() * 2;
    mesh.vertexCount = quads_to_render.size() * 4;

    mesh.vertices = (float *)MemAlloc(mesh.vertexCount*3*sizeof(float));    // 3 vertices, 3 coordinates each (x, y, z)
    mesh.texcoords = (float *)MemAlloc(mesh.vertexCount*2*sizeof(float));   // 3 vertices, 2 coordinates each (x, y)
    mesh.normals = (float *)MemAlloc(mesh.vertexCount*3*sizeof(float));     // 3 vertices, 3 coordinates each (x, y, z)
    mesh.indices = (unsigned short *)MemAlloc(quads_to_render.size()*6*sizeof(unsigned short)); // 6 indices per face
    mesh.colors = (unsigned char *)MemAlloc(mesh.vertexCount * 4* sizeof(char));

    for (Quad& quad : quads_to_render) {
      auto tex_coords = texture_atlas.lookup_texture_uv(
        quad.block_type,
        quad.direction
      );
      for (int i = 0; i < Quad::n_vertices; i ++) {
        mesh.vertices[vertex_index]          = quad.vertices[i].x;
        mesh.vertices[vertex_index + 1]      = quad.vertices[i].y;
        mesh.vertices[vertex_index + 2]      = quad.vertices[i].z;

        mesh.normals[normal_index]           = quad.normal.x;
        mesh.normals[normal_index + 1]       = quad.normal.y;
        mesh.normals[normal_index + 2]       = quad.normal.z;

        mesh.texcoords[tex_coords_index]     = tex_coords[i].x;
        mesh.texcoords[tex_coords_index + 1] = tex_coords[i].y;

        mesh.colors[colours_index    ]       = quad.color.r;
        mesh.colors[colours_index + 1]       = quad.color.g;
        mesh.colors[colours_index + 2]       = quad.color.b;
        mesh.colors[colours_index + 3]       = quad.color.a;

        normal_index     += 3;
        vertex_index     += 3;
        tex_coords_index += 2;
        colours_index    += 4;
      }
     /* we want all faces to be regular like this:
      0  1
      2  3
      triangle indexes are then:
      021 and  312
     */
      // triangle 021
      mesh.indices[indices_index]     = vertex_count;
      mesh.indices[indices_index + 1] = vertex_count + 2;
      mesh.indices[indices_index + 2] = vertex_count + 1;

      // triangle 312
      mesh.indices[indices_index + 3] = vertex_count + 3;
      mesh.indices[indices_index + 4] = vertex_count + 1;
      mesh.indices[indices_index + 5] = vertex_count + 2;
      indices_index += 6;
      vertex_count  += 4;
    }

    //todo: log info about the mesh
    // step 3:
    // upload to gpu
    // Upload mesh data from CPU (RAM) to GPU (VRAM) memory
    // todo: can this be done from a separate thread - probs main only.
    // todo: separate this step so that the mesh building can happen on a non main thread
    UploadMesh(&mesh, false);

    return mesh;
  }