#pragma once

#include <raylib.h>
#include "world/world.h"

// Kinematic, full-block AABB collision rules. No raylib window or GPU
// needed.
struct WorldPhysics {
  explicit WorldPhysics(const World& world) : world(world) {}

  bool is_grounded(const BoundingBox& player) const;

  // AIR never collides, irrespective of the geometry returned by
  // Block::bounds.
  bool player_overlaps_block(
    Vector3Int cell, BlockType block_type, const BoundingBox& player
  ) const;

  // Resolve each axis against a swept AABB. The caller does not supply
  // potentially stale collision candidates.
  void move_player(
    Vector3& feet, Vector3& velocity, Vector3 size, Vector3 displacement
  ) const;

private:
  const World& world;
};
