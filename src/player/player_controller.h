#pragma once

#include <raylib.h>

#include <vector>

#include "vector.h"
#include "world/world.h"

struct WorldPhysics;

struct PlayerMovementInput {
  Vector2 movement{};  // x: forward/back, y: strafe right/left
};

// First-person camera and kinematic player movement. All collision
// resolution and the grounded rule are provided by WorldPhysics.
struct PlayerController {
  // defined as the center (in xz plane) of the bottom (smallest y) of the bounds
  Vector3 feet{};
  Vector3 velocity{};
  bool grounded = false;



  PlayerController(
    const World &world,
    const WorldPhysics &world_physics,
    Camera3D &camera,
    Vector3 feet,
    Vector3 size
  );

  void apply_look(Vector2 mouse_delta);
  void queue_jump() noexcept { jump_buffer=jump_buffer_duration; }
  void fixed_update(const PlayerMovementInput& input,float dt);
  void reset(Vector3 feet);

  BoundingBox bounds() const;

  static constexpr float eye_height = 1.62f;
  std::vector<Vector3Int> blocks_overlapping_with_player() const;


private:
  void sync_camera();

  Vector3 size{};
  const World &world;
  const WorldPhysics &world_physics;
  Camera3D &camera;
  float yaw = 0.0f;
  float pitch = 0.0f;

  float jump_buffer = 0.0f;

  static constexpr float speed = 4.0f;
  static constexpr float gravity = -9.81f;
  static constexpr float jump_height = 1.25f;
  static constexpr float jump_buffer_duration = 0.1f;

};
