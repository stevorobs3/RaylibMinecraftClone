#pragma once

#include <vector>

#include "world/world.h"
#include "rendering/chunk_section_mesh.h"
#include "texture_atlas.h"


struct WorldMesh {

  // todo: this should probably handle dirty management
  // todo: can the world issue an event / callback when a chunk section changes?
  WorldMesh(World &world, TextureAtlas& texture_atlas);

  //todo: handle async things
  void rebuild_dirty_meshes();
  void unload();

  // todo: remove those that are:
  // 1) outside the camera (frustrum culling)
  // 2) not within the same air pocket as the camera

  std::vector<ChunkSectionMesh> chunk_section_meshes;


private:
  World &world;
  TextureAtlas& texture_atlas;

};