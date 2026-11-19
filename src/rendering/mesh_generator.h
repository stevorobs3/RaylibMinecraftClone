#pragma once
#include <optional>
#include <vector>
#include "raylib.h"
#include "world/world.h"
#include "texture_atlas.h"

// CPU-only geometry, usable without an OpenGL context.
struct MeshData {
  std::vector<float> vertices, texcoords, normals;
  std::vector<unsigned char> colors;
  std::vector<unsigned short> indices;
  int vertex_count = 0;
  int triangle_count = 0;
};

struct MeshGenerator {
  explicit MeshGenerator(TextureAtlas& atlas) : texture_atlas(atlas) {}
  std::optional<MeshData> build(const ChunkSection& section) const;
  static Mesh upload(const MeshData& data); // main thread only
  std::optional<Mesh> from(ChunkSection& section) const;
private:
  TextureAtlas& texture_atlas;
};
