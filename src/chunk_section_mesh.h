#pragma once

#include <cassert>
#include <optional>

#include "raylib.h"

#include "world.h"
#include "mesh_generator.h"

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
    std::optional<Mesh> mesh = mesh_generator.from(chunk_section);
    if(!mesh) {
      // its a logical error if the mesh is asked to be rebuilt, and it goes from
      // empty mesh -> empty mesh?
      // todo: ensure that when a neighbouring mesh has a block on the perimeter updated
      // that it only marks meshes dirty if they are non-empty.
      // todo: enable this assert
      // assert(model != std::nullopt);
      if (model) {
        UnloadModel(*model);
      }
      model = std::nullopt;
      return;
    }
    if (!model) {
      create_model(*mesh);
    }
    else {
      UnloadMesh(model->meshes[0]);
      model->meshes[0] = *mesh;
    }

  }

  std::optional<Model> model;
  ChunkSection &chunk_section;


private:

  MeshGenerator mesh_generator;
  TextureAtlas& texture_atlas;

};