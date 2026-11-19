#include "player_controller.h"

#include "raylib.h"
#include "world_physics.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>

PlayerController::PlayerController(
  const WorldPhysics &world_physics,
  Camera3D &camera,
  Vector3 feet,
  Vector3 size
) : world_physics(world_physics),
  feet(feet),
  size(size),
  camera(camera) {
  TraceLog(LOG_INFO, "Creating PlayerController");
  const Vector3 forward{
    camera.target.x - camera.position.x,
    camera.target.y - camera.position.y,
    camera.target.z - camera.position.z
  };
  const float length = std::sqrt(
    forward.x * forward.x + forward.y * forward.y + forward.z * forward.z
  );
  if (length > 0.0f) {
    yaw   = std::atan2(forward.z, forward.x);
    pitch = std::asin(std::clamp(forward.y / length, -1.0f, 1.0f));
  }
  grounded = world_physics.is_grounded(bounds());
  sync_camera();
}

void PlayerController::sync_camera() {
  const float cp  = std::cos(pitch);
  camera.position = {feet.x, feet.y + eye_height, feet.z};
  camera.target = {
    feet.x + cp * std::cos(yaw),
    feet.y + eye_height + std::sin(pitch),
    feet.z + cp * std::sin(yaw)
  };
}

void PlayerController::apply_look(Vector2 mouse_delta) {
  constexpr float sensitivity = 0.002f;
  constexpr float limit       = std::numbers::pi_v<float> / 2.0f - 0.01f;

  yaw += mouse_delta.x * sensitivity;
  pitch = std::clamp(pitch - mouse_delta.y * sensitivity, -limit, limit);
  sync_camera();
}

void PlayerController::reset(Vector3 initial_feet) {
  feet        = initial_feet;
  velocity    = {};
  jump_buffer = 0.0f;
  grounded    = world_physics.is_grounded(bounds());
  // Keep look direction through reset, as in the original game.
  sync_camera();
}

void PlayerController::fixed_update(const PlayerMovementInput& input,
  float dt) {

  // Horizontal movement is relative to yaw, not pitch.
  const float cy = std::cos(yaw);
  const float sy = std::sin(yaw);
  Vector3 horizontal_velocity{
    cy * input.movement.x - sy * input.movement.y,
    0.0f,
    sy * input.movement.x + cy * input.movement.y
  };
  const float length = std::sqrt(
    horizontal_velocity.x * horizontal_velocity.x + horizontal_velocity.z
      * horizontal_velocity.z
  );
  if (length > 0.0f) {
    horizontal_velocity.x *= speed / length;
    horizontal_velocity.z *= speed / length;
  }

  {
    const bool supported = world_physics.is_grounded(bounds());
    float vertical       = velocity.y;
    const bool can_jump  = supported && vertical <= 0.1f;

    if (jump_buffer > 0.0f && can_jump) {
      vertical    = std::sqrt(2.0f * -gravity * jump_height);
      jump_buffer = 0.0f;
    } else if (supported && vertical < 0.0f) {
      vertical = 0.0f;
    }

    // Analytic integration for constant gravity within each tick.
    const float dy = vertical * dt +
                     0.5f * gravity * dt * dt;
    velocity = {
      horizontal_velocity.x,
      vertical + gravity * dt,
      horizontal_velocity.z
    };

    world_physics.move_player(
      feet, velocity, size,
      {horizontal_velocity.x * dt, dy, horizontal_velocity.z * dt}
    );

    jump_buffer = std::max(0.0f, jump_buffer - dt);
  }

  grounded = world_physics.is_grounded(bounds()) && velocity.y <= 0.1f;
  sync_camera();
}

BoundingBox PlayerController::bounds() const {
  return {
    .min = {
      feet.x - size.x * 0.5f, feet.y, feet.z - size.z * 0.5f
    },
    .max = {
      feet.x + size.x * 0.5f, feet.y + size.y, feet.z + size.z * 0.5f
    }
  };
}

std::vector<Vector3Int> PlayerController::blocks_overlapping_with_player()
  const {
  const BoundingBox bb = bounds();
  std::vector<Vector3Int> blocks;
  const int min_x = static_cast<int>(std::floor(bb.min.x));
  const int min_y = std::max(0, static_cast<int>(std::floor(bb.min.y)));
  const int min_z = static_cast<int>(std::floor(bb.min.z));
  const int max_x = static_cast<int>(std::ceil(bb.max.x));
  const int max_y = std::min(World::height,
    static_cast<int>(std::ceil(bb.max.y)));
  const int max_z = static_cast<int>(std::ceil(bb.max.z));
  for (int x = min_x; x < max_x; x++)
    for (int y = min_y; y < max_y; y++)
      for (int z = min_z; z < max_z; z++)
        blocks.push_back({x, y, z});
  return blocks;
}
