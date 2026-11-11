#include <raylib.h>
#include "raymath.h"

#include "player.h"
#include "player_controller.h"
#include "player_input.h"
#include "world.h"
#include "world_physics.h"
#include "render.h"

Vector3 player_initial_position{World::width / 2,World::height / 8, World::width / 2};


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

int main() {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(1280, 720, "Raylib minecraft clone");
  DisableCursor();
  PollInputEvents();

  World world;
  WorldPhysics physics(world);
  Player player;

  
  const Camera3D camera{
    .position = player_initial_position + Vector3(0, PlayerController::eye_height, 0),
    .up = {0.0f, 1.0f, 0.0f},
    .fovy = 75.0f,
    .projection = CAMERA_PERSPECTIVE
  };

  PlayerController controller(
    camera,
    player_initial_position,
    {0.6f, 1.8f, 0.6f}
  );

  reset(world, player, controller, physics);
  Renderer renderer(world, player, controller);
  SetTargetFPS(60);
  bool show_physics_debug = false;

  while (!WindowShouldClose()) {
    const PlayerInput input = PlayerInput::poll();

    if (input.wants_toggle_debug()) {
      show_physics_debug = !show_physics_debug;
    }
    if (input.wants_reset()) {
      reset(world, player, controller, physics);
    }

    // The targeting ray sees the mouse movement of this frame.
    controller.apply_look(input.look_delta);
    player.find_target(world, controller.camera());
    player.manipulate_blocks(world, physics, controller, input);

    // Constant-acceleration movement and AABB collisions at 60 Hz.
    controller.update(input.movement_command(), GetFrameTime(), physics);

    BeginDrawing();
    ClearBackground(RAYWHITE);
    BeginMode3D(controller.camera());
    renderer.draw_world();
    renderer.draw_player();
    EndMode3D();
    renderer.draw_ui(show_physics_debug);
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
