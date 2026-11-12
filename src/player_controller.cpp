#include "player_controller.h"

#include "world_physics.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>

PlayerController::PlayerController(
  World &world, Camera3D camera, Vector3 feet, Vector3 size
) : world(world), feet(feet), size(size), camera(camera) {
  const Vector3 forward{
    camera.target.x - camera.position.x,
    camera.target.y - camera.position.y,
    camera.target.z - camera.position.z
  };
  const float length = std::sqrt(
    forward.x * forward.x + forward.y * forward.y + forward.z * forward.z
  );
  if (length > 0.0f) {
    yaw = std::atan2(forward.z, forward.x);
    pitch = std::asin(std::clamp(forward.y / length, -1.0f, 1.0f));
  }
  sync_camera();
}

void PlayerController::sync_camera() {
  const float cp = std::cos(pitch);
  camera.position = {feet.x, feet.y + eye_height, feet.z};
  camera.target = {
    feet.x + cp * std::cos(yaw),
    feet.y + eye_height + std::sin(pitch),
    feet.z + cp * std::sin(yaw)
  };
}

void PlayerController::apply_look(Vector2 mouse_delta) {
  constexpr float sensitivity = 0.002f;
  constexpr float limit = std::numbers::pi_v<float> / 2.0f - 0.01f;

  yaw += mouse_delta.x * sensitivity;
  pitch = std::clamp(pitch - mouse_delta.y * sensitivity, -limit, limit);
  sync_camera();
}

void PlayerController::reset(Vector3 feet, const WorldPhysics& world) {
  feet = feet;
  velocity = {};
  step_accumulator = 0.0f;
  jump_buffer = 0.0f;
  grounded = world.is_grounded(bounds());
  // Keep look direction through reset, as in the original game.
  sync_camera();
}

void PlayerController::update(
  const PlayerMovementInput& input,
  float frame_dt,
  const WorldPhysics& world
) {

  if (input.jump) {
    jump_buffer = jump_buffer_duration;
  }

  // Horizontal movement is relative to yaw, not pitch.
  const float cy = std::cos(yaw);
  const float sy = std::sin(yaw);
  Vector3 horizontal_velocity{
    cy * input.movement.x - sy * input.movement.y,
    0.0f,
    sy * input.movement.x + cy * input.movement.y
  };
  const float length = std::sqrt(
    horizontal_velocity.x * horizontal_velocity.x + horizontal_velocity.z * horizontal_velocity.z
  );
  if (length > 0.0f) {
    horizontal_velocity.x *= speed / length;
    horizontal_velocity.z *= speed / length;
  }

  step_accumulator += std::clamp(frame_dt, 0.0f, 0.1f);
  // todo: redo this to avoid tunneling
  while (step_accumulator >= fixed_dt) {
    const bool supported = world.is_grounded(bounds());
    float vertical = velocity.y;
    const bool can_jump = supported && vertical <= 0.1f;

    if (jump_buffer > 0.0f && can_jump) {
      vertical = std::sqrt(2.0f * -gravity * jump_height);
      jump_buffer = 0.0f;
    } else if (supported && vertical < 0.0f) {
      vertical = 0.0f;
    }

    // Analytic integration for constant gravity within each tick.
    const float dy = vertical * fixed_dt +
                     0.5f * gravity * fixed_dt * fixed_dt;
    velocity = {
      horizontal_velocity.x,
      vertical + gravity * fixed_dt,
      horizontal_velocity.z
    };

    world.move_player(
      feet, velocity, size,
      {horizontal_velocity.x * fixed_dt, dy, horizontal_velocity.z * fixed_dt}
    );

    jump_buffer = std::max(0.0f, jump_buffer - fixed_dt);
    step_accumulator -= fixed_dt;
  }

  grounded = world.is_grounded(bounds()) && velocity.y <= 0.1f;
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

std::vector<int> PlayerController::ints_between(double min, double max, int max_value) const {
  assert(max >= min);
  

  int floor_min = static_cast<int>(min);
  int floor_max = std::min(static_cast<int>(max), max_value);
  int count = (floor_max - floor_min) + 1;
  std::vector<int> ints{};
  for (int i = floor_min; i <= floor_max; i++) {
    ints.push_back(i);
  }

  // add 1 block barrior around player
  if (floor_min - 1 >= 0)
    ints.push_back(floor_min - 1);
  if (floor_max + 1 < max_value)
    ints.push_back(floor_max + 1);
  
  return ints;
}

std::vector<Vector3Int> PlayerController::blocks_overlapping_with_player() const {
  
  auto bb = bounds();
  std::vector<int> x_s = ints_between(bb.min.x, bb.max.x, world.width);
  std::vector<int> y_s = ints_between(bb.min.y, bb.max.y, world.height);
  std::vector<int> z_s = ints_between(bb.min.z, bb.max.z, world.width);
  std::vector<Vector3Int> blocks{};

  for (int x : x_s) {
    for (int y: y_s) {
      for (int z: z_s) {
        Vector3Int pos{x,y,z};
        if (world.contains(pos))
          blocks.push_back(pos);
      }
    }
  }
  return blocks;
}
