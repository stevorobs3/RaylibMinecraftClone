#pragma once

#include <vector>

#include <raylib.h>

#include "vector.h"

struct WorldPhysics;

struct PlayerMovementInput {
  Vector2 movement{}; // x: forward/back, y: right/left
};

// Camera and player kinematics. WorldPhysics resolves collisions.
struct PlayerController {
  // Centre of the player's feet on the X/Z plane.
  Vector3 feet{};
  Vector3 velocity{};
  bool grounded = false;

  PlayerController(
    const WorldPhysics& world_physics,
    Camera3D& camera,
    Vector3 feet,
    Vector3 size
  );

  void apply_look(Vector2 mouse_delta);
  void queue_jump() noexcept { jump_buffer = jump_buffer_duration; }
  void fixed_update(const PlayerMovementInput& input, float dt);
  void reset(Vector3 feet);

  BoundingBox bounds() const;

  static constexpr float eye_height = 1.62f;

  std::vector<Vector3Int> blocks_overlapping_with_player() const;

private:
  void sync_camera();

  Vector3 size{};
  const WorldPhysics& world_physics;
  Camera3D& camera;

  float yaw = 0.0f;
  float pitch = 0.0f;
  float jump_buffer = 0.0f;

  static constexpr float speed                = 4.0f;
  static constexpr float gravity              = -9.81f;
  static constexpr float jump_height          = 1.25f;
  static constexpr float jump_buffer_duration = 0.1f;
};
