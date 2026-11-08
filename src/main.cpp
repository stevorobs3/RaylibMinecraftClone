#include <raylib.h>
#include <rlgl.h>

#include <algorithm>
#include <numbers>

#include "player.h"
#include "world.h"
#include "render.h"
#include "physics/physics.h"

void reset(World& world, Player& player, PhysicsWorld& physics) {
  world.initialise();
  physics.set_blocks(world, PhysicsWorld::NON_MOVING);
  physics.reset_player({2.0f, 3.0f, 2.0f});
  player.position = physics.player_position();
  player.velocity = physics.player_velocity();
  player.grounded = physics.player_grounded();
  player.target.reset();
}

int main() {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(1280, 720, "Raylib minecraft clone");
  DisableCursor();
  PollInputEvents();

  PhysicsWorld physics;
  World world;
  Player player;

  physics.create_player(
    {2.0f, 3.0f, 2.0f},
    {0.6f, 1.8f, 0.6f},
    PhysicsWorld::MOVING
  );
  reset(world, player, physics);

  Renderer renderer(world, player);
  SetTargetFPS(60);
  Camera3D camera = player.get_camera();

  while (!WindowShouldClose()) {
    if (IsKeyPressed(KEY_R)) {
      reset(world, player, physics);
    }

    bool remove = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool place = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    bool jump = IsKeyPressed(KEY_SPACE);

    Vector2 mouse_delta = GetMouseDelta();
    Vector3 movement{
      static_cast<float>((IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) -
                         (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))),
      0.0f,
      static_cast<float>((IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) -
                         (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)))
    };

    constexpr float sensitivity = 0.002f;
    player.yaw += mouse_delta.x * sensitivity;
    player.pitch -= mouse_delta.y * sensitivity;
    constexpr float pitch_limit = std::numbers::pi_v<float> / 2.0f - 0.01f;
    player.pitch = std::clamp(player.pitch, -pitch_limit, pitch_limit);

    // Target using the same camera as the last rendered frame.
    player.find_target(physics, camera);
    if (player.manipulate_blocks(world, place, remove)) {
      physics.set_blocks(world, PhysicsWorld::NON_MOVING);
    }

    Vector3 horizontal = player.movement_velocity(movement, camera);

    // Substep variable frame times to keep collision detection stable.
    float remaining = std::min(GetFrameTime(), 0.1f);
    constexpr float max_step = 1.0f / 60.0f;
    while (remaining > 0.0f) {
      float dt = std::min(remaining, max_step);
      physics.update(dt);
      physics.move_player(horizontal, jump, dt);
      jump = false;
      remaining -= dt;
    }

    player.position = physics.player_position();
    player.velocity = physics.player_velocity();
    player.grounded = physics.player_grounded();
    camera = player.get_camera();

    BeginDrawing();
    ClearBackground(RAYWHITE);
    BeginMode3D(camera);
    renderer.draw_world();
    renderer.draw_player();
    EndMode3D();
    renderer.draw_ui();
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
