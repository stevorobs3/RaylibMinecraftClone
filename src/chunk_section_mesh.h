#pragma once

#include <cassert>
#include <optional>

#include "raylib.h"


#include "world.h"
#include "mesh_generator.h"

struct ChunkSectionMesh {

  ChunkSectionMesh(
    ChunkSection &chunk_section,
    Texture2D &texture
  ) : chunk_section(chunk_section), texture(texture) {

    std::optional<Mesh> mesh = mesh_generator.from(chunk_section);
    if (!mesh) return;

    create_model(*mesh);
  }

  void create_model(Mesh &mesh) {
    model = LoadModelFromMesh(mesh);
    model->materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
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
    if(!mesh) return;
    if (!model) {
      create_model(*mesh);
    }
    UnloadMesh(model->meshes[0]);
    model->meshes[0] = *mesh;

    // todo: use a texture atlas
    // todo: enable mipmaps, which requires work in the texture atlas to have spacing

  }

  std::optional<Model> model;
  ChunkSection &chunk_section;


private:

  MeshGenerator mesh_generator{};
  Texture2D& texture;

};