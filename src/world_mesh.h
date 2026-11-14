#pragma once

#include <vector>

#include "mesh_generator.h"
#include "raylib.h"
#include "world.h"
#include "chunk_section_mesh.h"



struct WorldMesh {

  // todo: this should probably handle dirty management
  // todo: can the world issue an event / callback when a chunk section changes?
  // todo: how to manage lookups
  WorldMesh(World &world) : world(world) {
    Image checked = GenImageChecked(2, 2, 1, 1, PINK, YELLOW);
    texture = LoadTextureFromImage(checked);
    UnloadImage(checked);


    chunk_section_meshes.reserve(world.num_chunks * Chunk::n_sections);
    for (int i = 0; i < world.num_chunks; i++) {
      for (int j = 0; j < Chunk::n_sections; j++) {
        ChunkSection &chunk_section = world.get_chunk_section(i,j);
        chunk_section_meshes.push_back(ChunkSectionMesh{chunk_section, texture});
      }
    }
  }

  //todo: handle async things
  void rebuild_dirty_meshes() {

    for (auto [i,j] : world.dirty_chunk_sections) {
      int index = i * Chunk::n_sections + j;
      TraceLog(LOG_INFO, std::format("rebuilding mesh:{},{}",i,j).c_str());
      chunk_section_meshes[index].rebuild_mesh();
    }
    world.dirty_chunk_sections.clear();
  }

  void unload() {
    for (ChunkSectionMesh &c : chunk_section_meshes) {
      c.unload();
    }
    UnloadTexture(texture);
  }

  // todo: remove those that are:
  // 1) outside the camera (frustrum culling)
  // 2) not within the same air pocket as the camera


  std::vector<ChunkSectionMesh> chunk_section_meshes;


private:
  World &world;
  MeshGenerator mesh_generator{};
  Texture texture;

};