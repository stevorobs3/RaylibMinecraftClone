
#include "game.h"

#include "raylib.h"
#include "raymath.h"
#include "player_input.h"
#include "rendering/texture_atlas.h"
#include <vector>
#include <optional>


Game::Game()
  : player_initial_position{
    0.5f,
    7.0f,
    0.5f
  },
  player_size{0.6f, 1.8f, 0.6f},
  camera{
    .position = player_initial_position + Vector3(
      0,
      PlayerController::eye_height,
      0
    ),
    .target = player_initial_position + Vector3(1.0f, PlayerController::eye_height, 0.0f),
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
  world_mesh.rebuild_all_meshes();
}
}

int Game::run()
 {
  if (!texture_atlas.load()) {
    TraceLog(LOG_ERROR, "texture atlas could not be loaded");
    return -1;
  }
  world_mesh.rebuild_all_meshes();

  bool show_physics_debug = false;


  while (!WindowShouldClose()) {
    const PlayerInput input = PlayerInput::poll();

    if (input.wants_toggle_debug()) {
      show_physics_debug = !show_physics_debug;
    }
    if (input.wants_reset()) {
      reset(world, world_mesh, player, controller, player_initial_position);
      fixed_stepper.reset();
    }

    // The targeting ray sees the mouse movement of this frame.

    controller.apply_look(input.look_delta);
    player.find_target(world, camera);
    player.manipulate_blocks(world, physics, controller.bounds(), input.block_action());

    // Multiple simulation systems can subscribe to this shared 60 Hz tick.
    if (input.jump) controller.queue_jump();
    fixed_stepper.advance(GetFrameTime(),[&](float dt) {
      controller.fixed_update(input.movement_command(),dt);
    });


    // todo: off main thread
    world_mesh.rebuild_dirty_meshes();

    BeginDrawing();
    ClearBackground(RAYWHITE);
    BeginMode3D(camera);

    renderer.draw_world();
    renderer.draw_player();

    EndMode3D();
    renderer.draw_ui(
      show_physics_debug,
      show_physics_debug ? controller.blocks_overlapping_with_player() : std::vector<Vector3Int>{}
    );
    EndDrawing();

  }

  return 0;
}

