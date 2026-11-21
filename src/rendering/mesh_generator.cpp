#include "mesh_generator.h"

#include <cstring>
#include <type_traits>

#include "face_appearance.h"
#include "quad.h"

namespace {

constexpr int max_vertices = 65536;

// UV2 is a sentinel on faces that have no overlay.
// One quad can contain both texture samples, so there is
// no coplanar geometry and no artificial vertex offset.
void append_quad(
  std::vector<MeshData>& meshes,
  const Quad& quad,
  const FaceAppearance& face,
  const TextureAtlas& atlas
) {
  if (meshes.empty() ||
      meshes.back().vertex_count + 4 > max_vertices) {
    meshes.emplace_back();
  }

  MeshData& mesh = meshes.back();

  const unsigned short base =
    static_cast<unsigned short>(mesh.vertex_count);
  const auto base_uv = atlas.lookup_texture_uv(face.base.texture);

  std::array<Vector2, 4> overlay_uv{{
    {-1.0f, -1.0f}, {-1.0f, -1.0f},
    {-1.0f, -1.0f}, {-1.0f, -1.0f}
  }};

  if (face.overlay) {
    overlay_uv = atlas.lookup_texture_uv(face.overlay->texture);
  }

  // With two layers, the base is opaque and the tint applies
  // to the overlay. Without it, tint the base as before.
  const Color tint = face.overlay
    ? face.overlay->tint : face.base.tint;

  for (int i = 0; i < Quad::n_vertices; i++) {
    const Vector3 v = quad.vertices[i];

    mesh.vertices.insert(
      mesh.vertices.end(), {v.x, v.y, v.z}
    );
    mesh.normals.insert(
      mesh.normals.end(),
      {quad.normal.x, quad.normal.y, quad.normal.z}
    );
    mesh.texcoords.insert(
      mesh.texcoords.end(), {base_uv[i].x, base_uv[i].y}
    );
    mesh.texcoords2.insert(
      mesh.texcoords2.end(), {overlay_uv[i].x, overlay_uv[i].y}
    );
    mesh.colors.insert(
      mesh.colors.end(), {tint.r, tint.g, tint.b, tint.a}
    );
  }

  mesh.indices.insert(mesh.indices.end(), {
    static_cast<unsigned short>(base),
    static_cast<unsigned short>(base + 2),
    static_cast<unsigned short>(base + 1),
    static_cast<unsigned short>(base + 3),
    static_cast<unsigned short>(base + 1),
    static_cast<unsigned short>(base + 2)
  });

  mesh.vertex_count   += 4;
  mesh.triangle_count += 2;
}

} // namespace

std::vector<MeshData> MeshGenerator::build(
  const ChunkSnapshot& snapshot
) const {
  std::vector<MeshData> result;

  for (int x = 0; x < ChunkSection::n; x++) {
    for (int y = 0; y < ChunkSection::n; y++) {
      for (int z = 0; z < ChunkSection::n; z++) {
        const BlockType type = snapshot.at(x, y, z);

        if (type == BlockType::AIR)
          continue;

        for (Direction dir : Quad::directions) {
          const Vector3 normal = Quad::lookup_normal(dir);

          const int dx = static_cast<int>(normal.x);
          const int dy = static_cast<int>(normal.y);
          const int dz = static_cast<int>(normal.z);

          if (snapshot.at(
            x + dx, y + dy, z + dz
          ) != BlockType::AIR) {
            continue;
          }

          const Quad quad{type, dir, {x, y, z}};
          const auto face = face_appearance(type, dir);

          append_quad(result, quad, face, atlas_);
        }
      }
    }
  }

  return result;
}

std::optional<Mesh> MeshGenerator::upload(const MeshData& data) {
  if (!data.valid())
    return std::nullopt;

  Mesh mesh{};
  mesh.vertexCount   = data.vertex_count;
  mesh.triangleCount = data.triangle_count;

  auto copy = [](const auto& src) {
    using T = typename std::decay_t<decltype(src)>::value_type;

    T* ptr = static_cast<T*>(
      MemAlloc(src.size() * sizeof(T))
    );

    if (ptr)
      std::memcpy(ptr, src.data(), src.size() * sizeof(T));

    return ptr;
  };

  mesh.vertices   = copy(data.vertices);
  mesh.texcoords  = copy(data.texcoords);
  mesh.texcoords2 = copy(data.texcoords2);
  mesh.normals    = copy(data.normals);
  mesh.colors     = copy(data.colors);
  mesh.indices    = copy(data.indices);

  if (!mesh.vertices || !mesh.texcoords || !mesh.texcoords2 ||
      !mesh.normals || !mesh.colors || !mesh.indices) {
    MemFree(mesh.vertices);
    MemFree(mesh.texcoords);
    MemFree(mesh.texcoords2);
    MemFree(mesh.normals);
    MemFree(mesh.colors);
    MemFree(mesh.indices);

    return std::nullopt;
  }

  UploadMesh(&mesh, false);

  if (mesh.vaoId == 0 || mesh.vboId == nullptr) {
    UnloadMesh(mesh);
    return std::nullopt;
  }

  return mesh;
}
