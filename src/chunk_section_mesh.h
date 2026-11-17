#pragma once

#include <cassert>
#include <optional>

#include "raylib.h"

#include "world.h"
#include "mesh_generator.h"
#include "timer.h"

struct ChunkSectionMesh {

  ChunkSectionMesh(
    ChunkSection &chunk_section,
    TextureAtlas &texture_atlas
  ): chunk_section(chunk_section),
     mesh_generator(texture_atlas),
     texture_atlas(texture_atlas) {

    std::optional<Mesh> mesh = mesh_generator.from(chunk_section);
    if (!mesh) return;

    create_model(*mesh);
  }

  void create_model(Mesh &mesh) {
    model = LoadModelFromMesh(mesh);
    model->materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture_atlas.texture;
  }

  void unload() {
    if (model)
      UnloadModel(*model);
  }

  //todo: this is slow and should be done off the main thread -> and some will need to
  // be done on the main thread (uploading to gpu)
  // rebuild the mesh for this chunk
  void rebuild_mesh() {

    // todo: is it better to unload the mesh or re-use it in place? (a later optimisation worth considering)
    auto start = Timer::now();
    std::optional<Mesh> mesh = mesh_generator.from(chunk_section);
    TraceLog(LOG_INFO, std::format(
      "Generated mesh in: {}",
      Timer::elapsed_ms(start)
      ).c_str()
    );

    if(!mesh) {
      assert(model != std::nullopt);
      if (model) {
        UnloadModel(*model);
      }
      model = std::nullopt;
      return;
    }
    TraceLog(LOG_INFO, std::format(
      "Unload old model in: {}",
      Timer::elapsed_ms(start)
      ).c_str()
    );
    if (!model) {
      create_model(*mesh);
      TraceLog(LOG_INFO, std::format(
        "Create new model in: {}",
        Timer::elapsed_ms(start)
      ).c_str()
    );
    }
    else {
      UnloadMesh(model->meshes[0]);
      model->meshes[0] = *mesh;
      TraceLog(LOG_INFO, std::format(
        "Unload old mesh in: {}",
        Timer::elapsed_ms(start)
      ).c_str()
    );
    }



  }

  std::optional<Model> model;
  ChunkSection &chunk_section;


private:

  MeshGenerator mesh_generator;
  TextureAtlas& texture_atlas;

};