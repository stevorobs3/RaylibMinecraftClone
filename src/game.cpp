
#include "game.h"

#include "raylib.h"
#include "raymath.h"
#include "player_input.h"
#include "rendering/texture_atlas.h"
#include "timer.h"
#include <optional>


Game::Game()
  : player_initial_position{
    World::width / 2.0f,
    1.0f,
    World::width / 2.0f
  },
  player_size{0.6f, 1.8f, 0.6f},
  camera{
    .position = player_initial_position + Vector3(
      0,
      PlayerController::eye_height,
      0
    ),
    .up = {0.0f, 1.0f, 0.0f},
    .fovy = 75.0f,
    .projection = CAMERA_PERSPECTIVE
  },
  physics(world),
  controller(world, physics, camera, player_initial_position, player_size),
  world_mesh(world, texture_atlas),
  renderer(world, world_mesh, player, controller)
   {
    TraceLog(LOG_INFO, "Creating Game");
   }

void Game::setup_raylib() {
  // Suppress raylib INFO logs and below
  // SetTraceLogLevel(LOG_WARNING);
  TraceLog(LOG_INFO, "setup raylib");
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(1280, 720, "Raylib minecraft clone");
  DisableCursor();
  PollInputEvents();
  SetTargetFPS(60);
}


namespace {
void reset(
  World& world,
  WorldMesh& world_mesh,
  Player& player,
  PlayerController& controller,
  Vector3 player_initial_position
) {
  world.initialise();
  controller.reset(player_initial_position);
  player.target.reset();
  world_mesh.rebuild_dirty_meshes();
}
}

int Game::run()
 {
  setup_raylib();

  if (!texture_atlas.load()) {
    TraceLog(LOG_ERROR, "texture atlas could not be loaded");
    return -1;
  }
  world_mesh.rebuild_all_meshes();

  bool show_physics_debug = false;

  auto timers = DebugTimers{};
  auto start = Timer::now();


  while (!WindowShouldClose()) {
    const PlayerInput input = PlayerInput::poll();

    if (input.wants_toggle_debug()) {
      show_physics_debug = !show_physics_debug;
    }
    if (input.wants_reset()) {
      reset(world, world_mesh, player, controller, player_initial_position);
    }

    // The targeting ray sees the mouse movement of this frame.

    start = Timer::now();
    controller.apply_look(input.look_delta);
    timers.apply_look = Timer::elapsed_ms(start);
    player.find_target(world, camera);
    timers.find_target = Timer::elapsed_ms(start);
    player.manipulate_blocks(world, physics, controller, input);
    timers.manipulate_blocks = Timer::elapsed_ms(start);

    // Constant-acceleration movement and AABB collisions at 60 Hz.
    controller.update(input.movement_command(), GetFrameTime());
    timers.controller_update = Timer::elapsed_ms(start);


    // todo: off main thread
    world_mesh.rebuild_dirty_meshes();
    timers.mesh_rebuild = Timer::elapsed_ms(start);

    BeginDrawing();
    ClearBackground(RAYWHITE);
    BeginMode3D(camera);

    renderer.draw_world();
    renderer.draw_player();
    // todo: this doesn't really represent render time
    timers.render = Timer::elapsed_ms(start);

    EndMode3D();
    renderer.draw_ui(
      show_physics_debug,
      timers,
      controller.blocks_overlapping_with_player()
    );
    EndDrawing();

  }

  world_mesh.unload();
  texture_atlas.unload();

  CloseWindow();
  return 0;
}

