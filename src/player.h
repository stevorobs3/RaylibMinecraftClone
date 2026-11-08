#pragma once

#include <raylib.h>
#include <raymath.h>
#include <optional>
#include <iostream>

#include "physics/physics.h"
#include "world.h"

struct Player {
  Vector3 position{};  // Centre of the feet, synchronised from Jolt
  Vector3 velocity{};
  bool grounded = false;
  float yaw = 0.0f;
  float pitch = 0.0f;

  BoundingBox get_bounds() const;
  Vector3 movement_velocity(Vector3 direction, const Camera& camera) const;

  std::optional<BlockHit> target = std::nullopt;
  void find_target(const PhysicsWorld& physics, const Camera& camera);
  bool manipulate_blocks(World& world, bool place, bool remove);

  Vector3 get_forward() const;
  Camera3D get_camera() const;

private:
  static constexpr float player_width = 0.6f;
  static constexpr float player_height = 1.8f;
  static constexpr float xz_movement_speed = 4.0f;
};

std::ostream& operator<<(std::ostream& os, const Player& player);
