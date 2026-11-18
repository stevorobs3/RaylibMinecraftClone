#pragma once

#include <cassert>
#include <optional>

#include "raylib.h"

#include "rendering/mesh_generator.h"

struct ChunkSectionMesh {

  ChunkSectionMesh(
    ChunkSection &chunk_section,
    TextureAtlas &texture_atlas
  );

  void create_model(Mesh &mesh);
  void unload();
  void rebuild_mesh();

  std::optional<Model> model;
  ChunkSection &chunk_section;

private:

  MeshGenerator mesh_generator;
  TextureAtlas& texture_atlas;

};