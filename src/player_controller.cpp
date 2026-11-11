#include "player_controller.h"

#include "world_physics.h"

#include <algorithm>
#include <cmath>
#include <numbers>

PlayerController::PlayerController(
  Camera3D camera, Vector3 feet, Vector3 size
) : feet_(feet), size_(size), camera_(camera) {
  const Vector3 forward{
    camera.target.x - camera.position.x,
    camera.target.y - camera.position.y,
    camera.target.z - camera.position.z
  };
  const float length = std::sqrt(
    forward.x * forward.x + forward.y * forward.y + forward.z * forward.z
  );
  if (length > 0.0f) {
    yaw_ = std::atan2(forward.z, forward.x);
    pitch_ = std::asin(std::clamp(forward.y / length, -1.0f, 1.0f));
  }
  sync_camera();
}

void PlayerController::sync_camera() {
  const float cp = std::cos(pitch_);
  camera_.position = {feet_.x, feet_.y + eye_height, feet_.z};
  camera_.target = {
    feet_.x + cp * std::cos(yaw_),
    feet_.y + eye_height + std::sin(pitch_),
    feet_.z + cp * std::sin(yaw_)
  };
}

void PlayerController::apply_look(Vector2 mouse_delta) {
  constexpr float sensitivity = 0.002f;
  constexpr float limit = std::numbers::pi_v<float> / 2.0f - 0.01f;

  yaw_ += mouse_delta.x * sensitivity;
  pitch_ = std::clamp(pitch_ - mouse_delta.y * sensitivity, -limit, limit);
  sync_camera();
}

void PlayerController::reset(Vector3 feet, const WorldPhysics& world) {
  feet_ = feet;
  velocity_ = {};
  step_accumulator_ = 0.0f;
  jump_buffer_ = 0.0f;
  jump_count_ = 0;
  recent_jumps_ = {};
  grounded_ = world.is_grounded(bounds());
  // Keep look direction through reset, as in the original game.
  sync_camera();
}

void PlayerController::update(
  const PlayerMovementInput& input,
  float frame_dt,
  const WorldPhysics& world
) {
  constexpr float speed = 4.0f;
  constexpr float gravity = -9.81f;
  constexpr float jump_height = 2.0f;
  constexpr float fixed_dt = 1.0f / 60.0f;
  constexpr float jump_buffer_duration = 0.1f;

  if (input.jump) {
    for (std::size_t i = recent_jumps_.size() - 1; i > 0; --i) {
      recent_jumps_[i] = recent_jumps_[i - 1];
    }
    recent_jumps_[0] = JumpPress{
      .feet_y = feet_.y,
      .vertical_velocity = velocity_.y,
      .grounded = world.is_grounded(bounds()) && velocity_.y <= 0.1f,
      .executed = false
    };
    jump_count_ = std::min(jump_count_ + 1, recent_jumps_.size());
    jump_buffer_ = jump_buffer_duration;
  }

  // Horizontal movement is relative to yaw, not pitch.
  const float cy = std::cos(yaw_);
  const float sy = std::sin(yaw_);
  Vector3 horizontal{
    cy * input.movement.x - sy * input.movement.y,
    0.0f,
    sy * input.movement.x + cy * input.movement.y
  };
  const float length = std::sqrt(
    horizontal.x * horizontal.x + horizontal.z * horizontal.z
  );
  if (length > 0.0f) {
    horizontal.x *= speed / length;
    horizontal.z *= speed / length;
  }

  step_accumulator_ += std::clamp(frame_dt, 0.0f, 0.1f);
  while (step_accumulator_ >= fixed_dt) {
    const bool supported = world.is_grounded(bounds());
    float vertical = velocity_.y;
    const bool can_jump = supported && vertical <= 0.1f;

    if (jump_buffer_ > 0.0f && can_jump) {
      vertical = std::sqrt(2.0f * -gravity * jump_height);
      jump_buffer_ = 0.0f;
      if (jump_count_ > 0) {
        recent_jumps_[0].executed = true;
      }
    } else if (supported && vertical < 0.0f) {
      vertical = 0.0f;
    }

    // Analytic integration for constant gravity within each tick.
    const float dy = vertical * fixed_dt +
                     0.5f * gravity * fixed_dt * fixed_dt;
    velocity_ = {
      horizontal.x,
      vertical + gravity * fixed_dt,
      horizontal.z
    };

    world.move_player(
      feet_, velocity_, size_,
      {horizontal.x * fixed_dt, dy, horizontal.z * fixed_dt}
    );

    jump_buffer_ = std::max(0.0f, jump_buffer_ - fixed_dt);
    step_accumulator_ -= fixed_dt;
  }

  grounded_ = world.is_grounded(bounds()) && velocity_.y <= 0.1f;
  sync_camera();
}

Vector3 PlayerController::position() const { return feet_; }
Vector3 PlayerController::velocity() const { return velocity_; }
bool PlayerController::grounded() const { return grounded_; }

BoundingBox PlayerController::bounds() const {
  return {
    .min = {
      feet_.x - size_.x * 0.5f, feet_.y, feet_.z - size_.z * 0.5f
    },
    .max = {
      feet_.x + size_.x * 0.5f, feet_.y + size_.y, feet_.z + size_.z * 0.5f
    }
  };
}

const Camera3D& PlayerController::camera() const { return camera_; }

std::array<PlayerController::JumpPress, 3>
PlayerController::recent_jump_presses() const {
  return recent_jumps_;
}

std::size_t PlayerController::jump_press_count() const {
  return jump_count_;
}
