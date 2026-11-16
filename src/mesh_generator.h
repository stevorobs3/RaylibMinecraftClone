#pragma once

#include "raylib.h"

#include <array>

#include "world.h"
#include "face.h"
#include "texture_atlas.h"
//todo: logic for neighbours or face normals are wrong here!


// todo: separate out into .h and .cpp
// todo: probably just a namespace?
struct MeshGenerator {

  MeshGenerator(TextureAtlas& texture_atlas) : texture_atlas(texture_atlas) {}

  // todo: calculate based on block_type and its location in the texture
  // todo: lookup these from the texture atlas


  // TODO: currently just renders a cube -> next stage iterate over all cubes
  // in the mesh adding the faces that aren't occluded by the neighbours
  std::optional<Mesh> from(ChunkSection &chunk_section) {

    // step 1:
    // build up a conceptual list of vector<tuple(FaceDirection,Position)>
    auto faces_to_render = faces_from(chunk_section);
    TraceLog(LOG_INFO, std::format(
      "faces_to_render:{} for {}",
      faces_to_render.size(),
      to_string(chunk_section.origin)).c_str());

    if (faces_to_render.empty()) {
      TraceLog(LOG_INFO, std::format("No mesh to render for:{}",to_string(chunk_section.origin)).c_str());
      return std::nullopt;
    }

    // step 2:
    // iterate over that list, generating all the mesh vertices

    int vertex_index = 0;
    int vertex_count = 0;
    int normal_index = 0;
    int tex_coords_index = 0;
    int indices_index = 0;

    Mesh mesh{};

    mesh.triangleCount = faces_to_render.size() * 2;
    mesh.vertexCount = faces_to_render.size() * 4;

    mesh.vertices = (float *)MemAlloc(mesh.vertexCount*3*sizeof(float));    // 3 vertices, 3 coordinates each (x, y, z)
    mesh.texcoords = (float *)MemAlloc(mesh.vertexCount*2*sizeof(float));   // 3 vertices, 2 coordinates each (x, y)
    mesh.normals = (float *)MemAlloc(mesh.vertexCount*3*sizeof(float));     // 3 vertices, 3 coordinates each (x, y, z)
    mesh.indices = (unsigned short *)MemAlloc(faces_to_render.size()*6*sizeof(unsigned short)); // 6 indices per face

    for (auto && [direction, position] : faces_to_render) {
      auto vertices = Face::vertices(direction);
      auto normal = Face::normal(direction);
      for (int i = 0; i < Face::n_vertices; i ++) {
        // todo: generate all vertices in local to ChunkSection space and use
        // the origin of the chunk_section when rendering
        mesh.vertices[vertex_index]          = vertices[i].x + position.x;
        mesh.vertices[vertex_index + 1]      = vertices[i].y + position.y;
        mesh.vertices[vertex_index + 2]      = vertices[i].z + position.z;
        mesh.normals[normal_index]           = normal.x;
        mesh.normals[normal_index + 1]       = normal.y;
        mesh.normals[normal_index + 2]       = normal.z;
        mesh.texcoords[tex_coords_index]     = all_tex_coords[i].x;
        mesh.texcoords[tex_coords_index + 1] = all_tex_coords[i].y;


        vertex_index  += 3;
        normal_index  += 3;
        tex_coords_index += 2;
      }
       /* we want all faces to be regular like this:
      0  1
      2  3
      triangle indexes are then:
      021 and  312
     */
      // triangle 021
      mesh.indices[indices_index] = vertex_count;
      mesh.indices[indices_index + 1] = vertex_count + 2;
      mesh.indices[indices_index + 2] = vertex_count + 1;

      // triangle 312
      mesh.indices[indices_index + 3] = vertex_count + 3;
      mesh.indices[indices_index + 4] = vertex_count + 1;
      mesh.indices[indices_index + 5] = vertex_count + 2;
      indices_index += 6;
      vertex_count += Face::n_vertices;

    }

    //todo: log info about the mesh
    // step 3:
    // upload to gpu
    // Upload mesh data from CPU (RAM) to GPU (VRAM) memory
    // todo: can this be done from a separate thread - probs main only.
    // todo: separate this step so that the mesh building can happen on a non main thread
    UploadMesh(&mesh, false);

    return mesh;
  }
private:
  TextureAtlas& texture_atlas;


  // todo: also will need to be aware of block shape (half block, stairs, etc)
  // todo: quesion: can this be reused for things like chests? or would that need to
  // be separate?
  std::vector<std::tuple<BlockType, Face::Direction, Vector3Int>> faces_from(
    ChunkSection chunk_section
  ) {
    std::vector<std::tuple<BlockType, Face::Direction,Vector3Int>> faces;

    auto blocks = chunk_section.blocks();
    // internal blocks can use the efficient lookup of blocks
    // todo: maybe have an iterator in the chunk_section for:
    // 1) internal blocks
    // 2) edge / perimiter / surface blocks (better name needed)
    for (int x = 1; x < ChunkSection::n - 1; x++) {
      for (int y = 1; y < ChunkSection::n - 1; y++) {
        for (int z = 1; z < ChunkSection::n - 1; z++) {
          if (blocks[x,y,z] == BlockType::Air) continue;
          for (Face::Direction dir : Face::directions) {
            auto neighbour = chunk_section.internal_neighbour(dir,{x,y,z});
            //todo: deeper checks here for different block shapes
            if (neighbour == BlockType::Air) {
              // todo: remove origin addition
              faces.push_back({
                blocks[x,y,z],
                dir,
                Vector3Int{x,y,z} + chunk_section.origin}
              );
            }
          }
        }
      }
    }

    // external blocks need to use world to check neighbouring chunks

    //todo: pull this into a reusable thing with std::views::iota for x,y and z
    // top and bottom faces
    for (int y : {0, ChunkSection::n - 1}) {
      for (int x = 0; x < ChunkSection::n; x++) {
        for (int z = 0; z < ChunkSection::n; z++) {
          // todo: remove origin
          auto block_position = Vector3Int(x,y,z) + chunk_section.origin;
          if (chunk_section.blocks()[x,y,z] == BlockType::Air) continue;
          for (Face::Direction dir : Face::directions) {
            auto neighbour_pos = block_position + Face::normal(dir);
            // todo: remove this once world is infinitely generated
            if (!chunk_section.world.contains(neighbour_pos))
              continue;
            // todo: why is `auto` coloured white - would be nice for it to be Blue / green
            auto neighbour = chunk_section.world.get(neighbour_pos);
            if (neighbour == BlockType::Air) {
              faces.push_back({
                chunk_section.world.get(block_position),
                dir,
                Vector3Int{x,y,z} + chunk_section.origin}
              );
            }
          }
        }
      }
    }

    // east and west faces
    for (int x : {0, ChunkSection::n - 1}) {
      // we've already done y = 0 and y = ChunkSection::n - 1 for top and bottom faces
      for (int y = 1; y < ChunkSection::n - 1; y++) {
        for (int z = 0; z < ChunkSection::n; z++) {
          auto block_position = Vector3Int(x,y,z) + chunk_section.origin;
          if (chunk_section.blocks()[x,y,z] == BlockType::Air) continue;
          for (Face::Direction dir : Face::directions) {
            auto neighbour_pos = block_position + Face::normal(dir);
            // todo: remove this once world is infinitely generated
            if (!chunk_section.world.contains(neighbour_pos))
              continue;
            auto neighbour = chunk_section.world.get(neighbour_pos);
            if (neighbour == BlockType::Air) {
              faces.push_back({
                chunk_section.world.get(block_position),
                dir,
                Vector3Int{x,y,z} + chunk_section.origin}
              );
            }
          }
        }
      }
    }


    // north and south faces
    for (int z : {0, ChunkSection::n - 1}) {
      // we've already done x = 0 and ChunkSection::n - 1 for east and west faces
      for (int x = 1; x < ChunkSection::n - 1; x++) {
        // we've already done y = 0 and y = ChunkSection::n - 1 for top and bottom faces
        for (int y = 1; y < ChunkSection::n - 1; y++) {
          auto block_position = Vector3Int(x,y,z) + chunk_section.origin;
          if (chunk_section.blocks()[x,y,z] == BlockType::Air) continue;
          for (Face::Direction dir : Face::directions) {
            auto neighbour_pos = block_position + Face::normal(dir);
            // todo: remove this once world is infinitely generated
            if (!chunk_section.world.contains(neighbour_pos))
              continue;
            auto neighbour = chunk_section.world.get(neighbour_pos);
            if (neighbour == BlockType::Air) {
              faces.push_back({
                chunk_section.world.get(block_position),
                dir,
                Vector3Int{x,y,z} + chunk_section.origin}
              );
            }
          }
        }
      }
    }

    return faces;
  }
};