#pragma once

#include "raylib.h"

#include "world/world.h"
#include "texture_atlas.h"


struct MeshGenerator {

  MeshGenerator(
    TextureAtlas& texture_atlas
  ) : texture_atlas(texture_atlas) {}

  std::optional<Mesh> from(ChunkSection &chunk_section);

private:
  TextureAtlas& texture_atlas;

};