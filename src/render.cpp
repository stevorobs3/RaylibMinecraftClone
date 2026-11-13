#include <raylib.h>
#include <rlgl.h>

#include "render.h"
#include "world.h"


void Renderer::draw_world() {
  // todo: use meshes instead of raw cubes here!

  for (auto [block_type, x, y, z] : world.block_types_with_positions()) {
    if (block_type != BlockType::Air) {
      const Color color = block_color.at(block_type);
      DrawCube(
        {x + 0.5f, y + 0.5f, z + 0.5f},
        1, 1, 1, color
      );
    }
  }
}

void Renderer::draw_player_target() {
  const auto& cell = player.target->position;

  rlDrawRenderBatchActive();
  const auto original_line_width = rlGetLineWidth();
  rlSetLineWidth(5.0f);
  DrawCubeWiresV(
    {cell.x + 0.5f, cell.y + 0.5f, cell.z + 0.5f},
    {1.0f, 1.0f, 1.0f},
    BLACK
  );
  rlDrawRenderBatchActive();
  rlSetLineWidth(original_line_width);
}

void Renderer::draw_player() {
  DrawBoundingBox(controller.bounds(), BLACK);
  if (player.target) {
    draw_player_target();
  }
}

void Renderer::draw_ui(
  bool show_physics_debug,
  DebugTimers debug_timers,
  std::vector<Vector3Int> blocks_ovelapping_player
) {
  DrawCircle(GetScreenWidth() / 2, GetScreenHeight() / 2, 3, BLACK);
  DrawFPS(0, 0);

  if (!show_physics_debug) {
    return;
  }

  Vector3 feet = controller.feet;
  Vector3 velocity = controller.velocity;
  DrawText(
    TextFormat(
      "Feet: %.3f,%.3f,%.3f",
      feet.x, feet.y, feet.z
    ),
    8, 35, 18, BLACK
  );

  DrawText(
    TextFormat(
      "timers: \napply_look:%.1f\nfind_target:%.1f\nmanipulate_blocks:%.1f\ncontroller_update:%.1f\nrender:%.1f",
      debug_timers.apply_look, debug_timers.find_target, debug_timers.manipulate_blocks, debug_timers.controller_update, debug_timers.render
    ),
    8, 55, 18, BLACK
  );

  std::string s = std::format("blocks:{}", blocks_ovelapping_player.size());
  for (auto block : blocks_ovelapping_player) {
    s += std::format("\n{},{},{}",block.x,block.y,block.z);
  }

  DrawText(
    s.c_str(),
    250, 35, 18, BLACK
  );
}

struct Face {
  // anti-clockwise Right hand wind
  static constexpr int n_vertices = 4;
  std::array<Vector3, 4> vertices;
  Vector3 normal;
  std::array<Vector2, 4> tex_coords;
};

Mesh Renderer::GenerateMesh(ChunkSection chunk_section) {

  std::array<Vector2, Face::n_vertices> all_tex_coords{
    Vector2{0,1},
    Vector2{1,1},
    Vector2{0,0},
    Vector2{1,0},
  };



  /* we want all faces to be regular like this:
    
    0  1
    2  3
    triangle indexes are then:
    021 and  312
   */

  std::array<Vector3,Face::n_vertices> pos_z_face{
    Vector3{0,1,1},
    Vector3{1,1,1},
    Vector3{0,0,1}, 
    Vector3{1,0,1},
  };

  // std::array<Vector3,6> neg_z_face{
  //   // tri-1
  //   Vector3{0,0,0},
  //   Vector3{0,1,0},
  //   Vector3{1,0,0},
    

  //   // tri-2
  //   Vector3{1,0,0},
  //   Vector3{0,1,0},
  //   Vector3{1,1,0},
    
    
  // };

  // std::array<Vector3,6> pos_x_face{
  //   // tri-1
  //   Vector3{1,0,0},
  //   Vector3{1,1,0},
  //   Vector3{1,0,1},
    
  //   // tri-2
  //   Vector3{1,0,1},
  //   Vector3{1,1,0},
  //   Vector3{1,1,1},
    
  // };

  // std::array<Vector3,6> neg_x_face{
  //   // tri-1
  //   Vector3{0,0,0},
  //   Vector3{0,0,1},
  //   Vector3{0,1,0},
    
  //   // tri-2
  //   Vector3{0,0,1},
  //   Vector3{0,1,1},
  //   Vector3{0,1,0},
    
    
  // };

  // std::array<Vector3,6> pos_y_face{
  //   // tri-1
  //   Vector3{0,1,0},
  //   Vector3{0,1,1},
  //   Vector3{1,1,0},
    
    
  //   // tri-2
  //   Vector3{0,1,1},
  //   Vector3{1,1,1},
  //   Vector3{1,1,0},
    
  // };

  // std::array<Vector3,6> neg_y_face{
  //   // tri-1
  //   Vector3{0,0,0},
  //   Vector3{1,0,0},
  //   Vector3{0,0,1},
    
    
  //   // tri-2
  //   Vector3{0,0,1},
  //   Vector3{1,0,0},
  //   Vector3{1,0,1},
    
  // };

  Vector3 pos_z_normal = Vector3(0,0,1);
  Vector3 neg_z_normal = Vector3(0,0,-1);

  Vector3 pos_x_normal = Vector3(1,0,0);
  Vector3 neg_x_normal = Vector3(-1,0,0);
  Vector3 pos_y_normal = Vector3(0,1,0);
  Vector3 neg_y_normal = Vector3(0,-1,0);



  // todo: all 6 faces for a block
  std::vector<Face> faces{
    Face{pos_z_face, pos_z_normal, all_tex_coords},
    // Face{neg_z_face, neg_z_normal, all_tex_coords},

    // Face{pos_x_face, pos_x_normal, all_tex_coords},
    // Face{neg_x_face, neg_x_normal, all_tex_coords},

    // Face{pos_y_face, pos_y_normal, all_tex_coords},
    // Face{neg_y_face, neg_y_normal, all_tex_coords},


  };

  int vertex_index = 0;
  int vertex_count = 0;
  int normal_index = 0;
  int tex_coords_index = 0;
  int indices_index = 0;

  Mesh mesh{};

  mesh.triangleCount = faces.size() * 2;
  mesh.vertexCount = faces.size() * 4;

  mesh.vertices = (float *)MemAlloc(mesh.vertexCount*3*sizeof(float));    // 3 vertices, 3 coordinates each (x, y, z)
  // mesh.texcoords = (float *)MemAlloc(mesh.vertexCount*2*sizeof(float));   // 3 vertices, 2 coordinates each (x, y)
  mesh.normals = (float *)MemAlloc(mesh.vertexCount*3*sizeof(float));     // 3 vertices, 3 coordinates each (x, y, z)
  mesh.indices = (unsigned short *)MemAlloc(faces.size()*6*sizeof(unsigned short *)); // 6 indices per face

  for (Face face : faces) {
    for (int i = 0; i < Face::n_vertices; i ++) {
      // todo: remove offset
      mesh.vertices[vertex_index] = face.vertices[i].x + 14;
      mesh.vertices[vertex_index + 1] = face.vertices[i].y + 2;
      mesh.vertices[vertex_index + 2] = face.vertices[i].z + 14;
      mesh.normals[normal_index] = face.normal.x;
      mesh.normals[normal_index + 1] = face.normal.y;
      mesh.normals[normal_index + 2] = face.normal.z;
      // mesh.texcoords[tex_coords_index] = face.tex_coords[i].x;
      // mesh.texcoords[tex_coords_index + 1] = face.tex_coords[i].y;


      vertex_index  += 3;
      normal_index  += 3;
      // tex_coords_index += 2;
    }
    // triangle 021
    mesh.indices[indices_index] = vertex_count;
    mesh.indices[indices_index] = vertex_count + 2;
    mesh.indices[indices_index] = vertex_count + 1;

    // triangle 312
    mesh.indices[indices_index] = vertex_count + 3;
    mesh.indices[indices_index] = vertex_count + 1;
    mesh.indices[indices_index] = vertex_count + 2;
    indices_index += 6;
    vertex_count += Face::n_vertices;

  }

  std::cout << std::format(
    "mesh: {}, {}, {}, {}\n",
     vertex_count,
     normal_index,
     vertex_index,
     indices_index);
// 4 12 12 6

  // Upload mesh data from CPU (RAM) to GPU (VRAM) memory
  UploadMesh(&mesh, false);

  return mesh;
}