#pragma once

#include <cassert>

#include "raylib.h"


#include "world.h"
#include "mesh_generator.h"

// a mesh for an chunk section
struct ChunkSectionMesh {

  ChunkSectionMesh(ChunkSection &chunk_section) : chunk_section(chunk_section) {
    // todo: pass this in
    Image checked = GenImageChecked(2, 2, 1, 1, PINK, YELLOW);
    texture = LoadTextureFromImage(checked);
    UnloadImage(checked);

    Mesh mesh = mesh_generator.from(chunk_section);
    model = LoadModelFromMesh(mesh);
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;


    rebuild_mesh();
  }

  void unload() {
    UnloadTexture(texture);
    UnloadModel(model);
  }

  //todo: this is slow and should be done off the main thread -> and some will need to
  // be done on the main thread (uploading to gpu)
  // rebuild the mesh for this chunk
  void rebuild_mesh() {

    // todo: is it better to unload the mesh or re-use it in place? (a later optimisation worth considering)
    Mesh mesh = mesh_generator.from(chunk_section);

    UnloadMesh(model.meshes[0]);
    model.meshes[0] = mesh;

    // todo: use a texture atlas
    // todo: enable mipmaps, which requires work in the texture atlas to have spacing

  }

  Model model;
  ChunkSection &chunk_section;


private:

  MeshGenerator mesh_generator{};
  Texture2D texture;

};