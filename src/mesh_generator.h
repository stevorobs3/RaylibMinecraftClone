#pragma once

#include "raylib.h"

#include <array>
#include <unordered_map>

#include "world.h"

struct Face {
  // anti-clockwise Right hand wind
  static constexpr int n_vertices = 4;
  std::array<Vector3, 4> vertices;
  Vector3 normal;
};

enum class FaceDirection {
  POS_X,
  NEG_X,
  POS_Y,
  NEG_Y,
  POS_Z,
  NEG_Z
};


struct MeshGenerator {

  // todo: calculate based on block_type and its location in the texture
  // todo: lookup these from the texture atlas


  // TODO: currently just renders a cube -> next stage iterate over all cubes
  // in the mesh adding the faces that aren't occluded by the neighbours
  Mesh from(ChunkSection &chunk_section) {

    int vertex_index = 0;
    int vertex_count = 0;
    int normal_index = 0;
    int tex_coords_index = 0;
    int indices_index = 0;

    Mesh mesh{};

    mesh.triangleCount = faces.size() * 2;
    mesh.vertexCount = faces.size() * 4;

    mesh.vertices = (float *)MemAlloc(mesh.vertexCount*3*sizeof(float));    // 3 vertices, 3 coordinates each (x, y, z)
    mesh.texcoords = (float *)MemAlloc(mesh.vertexCount*2*sizeof(float));   // 3 vertices, 2 coordinates each (x, y)
    mesh.normals = (float *)MemAlloc(mesh.vertexCount*3*sizeof(float));     // 3 vertices, 3 coordinates each (x, y, z)
    mesh.indices = (unsigned short *)MemAlloc(faces.size()*6*sizeof(unsigned short)); // 6 indices per face

    for (auto && [blockFace, face] : faces) {
      for (int i = 0; i < Face::n_vertices; i ++) {
        mesh.vertices[vertex_index] = face.vertices[i].x;
        mesh.vertices[vertex_index + 1] = face.vertices[i].y;
        mesh.vertices[vertex_index + 2] = face.vertices[i].z;
        mesh.normals[normal_index] = face.normal.x;
        mesh.normals[normal_index + 1] = face.normal.y;
        mesh.normals[normal_index + 2] = face.normal.z;
        mesh.texcoords[tex_coords_index] = all_tex_coords[i].x;
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

    // Upload mesh data from CPU (RAM) to GPU (VRAM) memory
    // todo: can this be done from a separate thread - probs main only.
    UploadMesh(&mesh, false);

    return mesh;
  }
private:
  std::array<Vector2, Face::n_vertices> all_tex_coords{
    Vector2{0,1},
    Vector2{1,1},
    Vector2{0,0},
    Vector2{1,0},
  };

  using FaceVertices = std::array<Vector3,Face::n_vertices>;

  // todo: generate these 6 once, and add the world position to each block
  FaceVertices pos_z_face{
    Vector3{0,1,1},
    Vector3{1,1,1},
    Vector3{0,0,1},
    Vector3{1,0,1},
  };

  FaceVertices neg_z_face{
    Vector3{1,1,0},
    Vector3{0,1,0},
    Vector3{1,0,0},
    Vector3{0,0,0},
  };

  FaceVertices pos_x_face{
    Vector3{1,1,1},
    Vector3{1,1,0},
    Vector3{1,0,1},
    Vector3{1,0,0},
  };

  FaceVertices neg_x_face{
    Vector3{0,1,0},
    Vector3{0,1,1},
    Vector3{0,0,0},
    Vector3{0,0,1},
  };

  FaceVertices pos_y_face{
    Vector3{1,1,0},
    Vector3{1,1,1},
    Vector3{0,1,0},
    Vector3{0,1,1},
  };

  FaceVertices neg_y_face{
    Vector3{1,0,1},
    Vector3{1,0,0},
    Vector3{0,0,1},
    Vector3{0,0,0},
  };

  std::unordered_map<FaceDirection, Face> faces{
    {FaceDirection::POS_X, Face{pos_x_face, {1,0,0}}},
    {FaceDirection::POS_X, Face{neg_x_face, {-1,0,0}}},

    {FaceDirection::POS_Y, Face{pos_y_face, {0,1,0}}},
    {FaceDirection::POS_Y, Face{neg_y_face, {0,-1,0}}},

    {FaceDirection::POS_Z, Face{pos_z_face, {0,0,1}}},
    {FaceDirection::NEG_Z, Face{neg_z_face, {0,0,-1}}},
  };
};