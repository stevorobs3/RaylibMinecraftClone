#pragma once

#include <raylib.h>

#include <vector>

#include "vector.h"
#include "world.h"

struct WorldPhysics;

struct PlayerMovementInput {
  Vector2 movement{};  // x: forward/back, y: strafe right/left
  bool jump = false;
};

// First-person camera and kinematic player movement. All collision
// resolution and the grounded rule are provided by WorldPhysics.
struct PlayerController {
  // defined as the center (in xz plane) of the bottom (smallest y) of the bounds
  Vector3 feet{};
  Vector3 velocity{};
  bool grounded = false;
  Camera3D camera{};
  
  
  
  PlayerController(World &world, Camera3D camera, Vector3 feet, Vector3 size);

  void apply_look(Vector2 mouse_delta);
  void update(
    const PlayerMovementInput& input,
    float frame_dt,
    const WorldPhysics& world
  );
  void reset(Vector3 feet, const WorldPhysics& world);

  BoundingBox bounds() const;
  
  static constexpr float eye_height = 1.62f;
  std::vector<Vector3Int> blocks_overlapping_with_player() const;

private:
  void sync_camera();

  std::vector<int> ints_between(double min, double max, int max_value) const;

  Vector3 size{};
  World &world;
  float yaw = 0.0f;
  float pitch = 0.0f;
  
  // needed for fixed_update() at 60FPS
  float step_accumulator = 0.0f;
  float jump_buffer = 0.0f;

  static constexpr float speed = 4.0f;
  static constexpr float gravity = -9.81f;
  static constexpr float jump_height = 1.25f;
  static constexpr float fixed_dt = 1.0f / 60.0f;
  static constexpr float jump_buffer_duration = 0.1f;

};
