#pragma once
#include <optional>
#include <vector>
#include <raylib.h>
#include "world/chunk_section.h"
#include "chunk_snapshot.h"
#include "texture_atlas.h"

struct MeshData {
  std::vector<float> vertices,texcoords,normals;
  std::vector<unsigned char> colors;
  std::vector<unsigned short> indices;
  int vertex_count=0;
  int triangle_count=0;
  bool valid() const noexcept {
    if(vertex_count<=0 || vertex_count>65536 || triangle_count<=0) return false;
    const auto vertices_count=static_cast<std::size_t>(vertex_count);
    const auto triangles_count=static_cast<std::size_t>(triangle_count);
    if(vertices.size()!=vertices_count*3 ||
       texcoords.size()!=vertices_count*2 ||
       normals.size()!=vertices_count*3 ||
       colors.size()!=vertices_count*4 ||
       indices.size()!=triangles_count*3) return false;
    for(unsigned short index:indices)
      if(index>=vertex_count) return false;
    return true;
  }
};
struct MeshGenerator {
  explicit MeshGenerator(const TextureAtlas& atlas) noexcept : atlas_(atlas) {}
  // Pure CPU; returns as many 16-bit-index-safe submeshes as needed.
  std::vector<MeshData> build(const ChunkSnapshot& snapshot) const;
  // GPU upload must happen on the graphics thread. Caller owns returned Mesh.
  static std::optional<Mesh> upload(const MeshData& data);
private:
  const TextureAtlas& atlas_;
};
