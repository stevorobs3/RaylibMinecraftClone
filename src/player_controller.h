#pragma once

#include <raylib.h>

#include <array>
#include <cstddef>

struct WorldPhysics;

struct PlayerMovementInput {
  Vector2 movement{};  // x: forward/back, y: strafe right/left
  bool jump = false;
};

// First-person camera and kinematic player movement. All collision
// resolution and the grounded rule are provided by WorldPhysics.
struct PlayerController {
  struct JumpPress {
    float feet_y = 0.0f;
    float vertical_velocity = 0.0f;
    bool grounded = false;
    bool executed = false;
  };

  PlayerController(Camera3D camera, Vector3 feet, Vector3 size);

  void apply_look(Vector2 mouse_delta);
  void update(
    const PlayerMovementInput& input,
    float frame_dt,
    const WorldPhysics& world
  );
  void reset(Vector3 feet, const WorldPhysics& world);

  Vector3 position() const;
  Vector3 velocity() const;
  bool grounded() const;
  BoundingBox bounds() const;
  const Camera3D& camera() const;

  std::array<JumpPress, 3> recent_jump_presses() const;
  std::size_t jump_press_count() const;

private:
  void sync_camera();

  Vector3 feet_{};
  Vector3 velocity_{};
  Vector3 size_{};
  bool grounded_ = false;
  Camera3D camera_{};
  float yaw_ = 0.0f;
  float pitch_ = 0.0f;
  float step_accumulator_ = 0.0f;
  float jump_buffer_ = 0.0f;

  std::array<JumpPress, 3> recent_jumps_{};
  std::size_t jump_count_ = 0;
};
