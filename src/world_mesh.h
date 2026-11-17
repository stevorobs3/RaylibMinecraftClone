#pragma once

#include <vector>

#include "world.h"
#include "chunk_section_mesh.h"
#include "texture_atlas.h"


struct WorldMesh {

  // todo: this should probably handle dirty management
  // todo: can the world issue an event / callback when a chunk section changes?
  WorldMesh(World &world, TextureAtlas& texture_atlas)
    : world(world), texture_atlas(texture_atlas) {

    // todo: more generally - what happens if something like the push_back fails?
    // todo: more generally - what error handling am I missing?
    // todo: more generally - the mesh generation requires run-time allocations, can this be mitigated / minimised / correctly error handled?
    chunk_section_meshes.reserve(world.num_chunks * Chunk::n_sections);
    for (int i = 0; i < world.num_chunks; i++) {
      for (int j = 0; j < Chunk::n_sections; j++) {
        ChunkSection &chunk_section = world.get_chunk_section(i,j);
        chunk_section_meshes.push_back(ChunkSectionMesh{
          chunk_section,
          texture_atlas
        });
      }
    }
  }

  //todo: handle async things
  void rebuild_dirty_meshes() {

    for (auto [i,j] : world.dirty_chunk_sections) {
      int index = i * Chunk::n_sections + j;
      assert(index >= 0 && index < chunk_section_meshes.size());
      chunk_section_meshes[index].rebuild_mesh();
    }
    world.dirty_chunk_sections.clear();
  }

  void unload() {
    for (ChunkSectionMesh &c : chunk_section_meshes) {
      c.unload();
    }
  }

  // todo: remove those that are:
  // 1) outside the camera (frustrum culling)
  // 2) not within the same air pocket as the camera


  std::vector<ChunkSectionMesh> chunk_section_meshes;


private:
  World &world;
  TextureAtlas& texture_atlas;

};