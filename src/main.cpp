#include <raylib.h>
#include "raymath.h"
#include <chrono>

#include "player.h"
#include "player_controller.h"
#include "player_input.h"
#include "world.h"
#include "world_physics.h"
#include "render.h"

Vector3 player_initial_position{World::width / 2.0f, 1.0f, World::width / 2.0f};


void reset(
  World& world,
  Player& player,
  PlayerController& controller,
  const WorldPhysics& physics
) {
  world.initialise();
  controller.reset(player_initial_position, physics);
  player.target.reset();
}

auto now() {
  return std::chrono::steady_clock::now();
}

double elapsed_ms(auto & start) {
  double duration =  std::chrono::duration<double, std::milli>(
        now() - start
  ).count();
  start = now();
  return duration;
}

int main() {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(1280, 720, "Raylib minecraft clone");
  DisableCursor();
  PollInputEvents();

  World world;
  Player player;

  
  const Camera3D camera{
    .position = player_initial_position + Vector3(0, PlayerController::eye_height, 0),
    .up = {0.0f, 1.0f, 0.0f},
    .fovy = 75.0f,
    .projection = CAMERA_PERSPECTIVE
  };

  PlayerController controller(
    world,
    camera,
    player_initial_position,
    {0.6f, 1.8f, 0.6f}
  );
  WorldPhysics physics(world, controller);
  

  reset(world, player, controller, physics);
  Renderer renderer(world, player, controller);
  SetTargetFPS(60);
  bool show_physics_debug = false;

  auto timers = DebugTimers{};
  auto start = now();

  // mesh generation testing...

  // todo: pull this into renderer somehow
  /// todo: use actual textures
  // todo: so many things
  Model model;
  Image checked = GenImageChecked(2, 2, 1, 1, PINK, YELLOW);
  Texture2D texture = LoadTextureFromImage(checked);
  UnloadImage(checked);
  // todo: render meshes for all the chunk sections in the world
  // todo: remove those that are:
  // 1) outside the camera (frustrum culling)
  // 2) not within the same air pocket as the camera
  model = LoadModelFromMesh(renderer.GenerateMesh(world.test_chunk_section()));
  // todo: use a texture atlas
  // todo: enable mipmaps, which requires work in the texture atlas to have spacing
  model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;


  while (!WindowShouldClose()) {
    const PlayerInput input = PlayerInput::poll();

    if (input.wants_toggle_debug()) {
      show_physics_debug = !show_physics_debug;
    }
    if (input.wants_reset()) {
      reset(world, player, controller, physics);
    }

    // The targeting ray sees the mouse movement of this frame.
    
    start = now();
    controller.apply_look(input.look_delta);
    timers.apply_look = elapsed_ms(start);
    player.find_target(world, controller.camera);
    timers.find_target = elapsed_ms(start);
    player.manipulate_blocks(world, physics, controller, input);
    timers.manipulate_blocks = elapsed_ms(start);
    
    // Constant-acceleration movement and AABB collisions at 60 Hz.
    controller.update(input.movement_command(), GetFrameTime(), physics);
    timers.controller_update = elapsed_ms(start);


    std::vector<Vector3Int> blocks_overlapping_player = controller.blocks_overlapping_with_player();
    BeginDrawing();
    ClearBackground(RAYWHITE);
    BeginMode3D(controller.camera);
    renderer.draw_world();
    renderer.draw_player();

    DrawModel(model, Vector3(14,2,14), 1.0f, WHITE);
    DrawModel(model, Vector3(15,2,14), 1.0f, WHITE);


    EndMode3D();
    timers.render = elapsed_ms(start);
    renderer.draw_ui(show_physics_debug, timers, blocks_overlapping_player);
    EndDrawing();
    
  }

  UnloadTexture(texture);
  UnloadModel(model);


  CloseWindow();
  return 0;
}
