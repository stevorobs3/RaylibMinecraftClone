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
