#include <raylib.h>

#include "player.h"
#include "player_input.h"
#include "world.h"
#include "world_physics.h"
#include "render.h"
#include "physics/physics.h"

void reset(
  World& world,
  WorldPhysics& colliders,
  Player& player,
  PlayerController& controller
) {
  world.initialise();
  colliders.rebuild(world);
  controller.reset({2.0f, 3.0f, 2.0f});
  player.target.reset();
}

int main() {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(1280, 720, "Raylib minecraft clone");
  DisableCursor();
  PollInputEvents();

  PhysicsWorld physics;
  World world;
  WorldPhysics colliders(physics, PhysicsWorld::NON_MOVING);
  Player player;

  Camera3D camera{
    .position = {2.0f, 4.62f, 2.0f},
    .target = {3.0f, 4.62f, 2.0f},
    .up = {0.0f, 1.0f, 0.0f},
    .fovy = 75.0f,
    .projection = CAMERA_PERSPECTIVE
  };
  PlayerController controller(
    physics,
    camera,
    {2.0f, 3.0f, 2.0f},
    {0.6f, 1.8f, 0.6f},
    PhysicsWorld::MOVING
  );

  reset(world, colliders, player, controller);
  Renderer renderer(world, player, controller);
  SetTargetFPS(60);
  bool show_physics_debug = false;

  while (!WindowShouldClose()) {
    PlayerInput input = PlayerInput::poll();
    if (input.wants_toggle_debug()) {
      show_physics_debug = !show_physics_debug;
    }

    if (input.wants_reset()) {
      reset(world, colliders, player, controller);
    }

    // Apply this frame's mouse look before raycasting.
    controller.apply_look(input.look_delta);
    player.find_target(physics, colliders, controller.camera());
    player.manipulate_blocks(world, colliders, input, controller);

    // Jolt advances on fixed 60 Hz ticks, independent of render FPS.
    controller.update(input.movement_command(), GetFrameTime());

    BeginDrawing();
    ClearBackground(RAYWHITE);
    BeginMode3D(controller.camera());
    renderer.draw_world();
    renderer.draw_player(show_physics_debug);
    EndMode3D();
    renderer.draw_ui(show_physics_debug);
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
