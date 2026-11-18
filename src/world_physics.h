#pragma once

#include <raylib.h>
#include "world/world.h"

// Minecraft-specific geometry rules for the current full-cube world.
// No general-purpose physics engine, rigid bodies, or collider ownership.
struct WorldPhysics {
  explicit WorldPhysics(
    const World& world
  ) : world(world) {
    TraceLog(LOG_INFO, "Creating WorldPhysics");
  }

  bool is_grounded(
    const BoundingBox player,
    const std::vector<Vector3Int> &blocks_overlapping_with_player
  ) const;

  bool player_overlaps_block(
    Vector3Int cell,
    BlockType block_type,
    const BoundingBox player_bounds
  ) const;

  // Resolve the intended displacement one axis at a time, sliding
  // along walls and cancelling velocity on blocked axes.
  void move_player(
    Vector3& feet,
    Vector3& velocity,
    Vector3 size,
    Vector3 displacement,
    const std::vector<Vector3Int> &blocks_overlapping_with_player
  ) const;

private:
  const World& world;
};
