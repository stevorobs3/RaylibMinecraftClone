#pragma once

#include <raylib.h>
#include "player_controller.h"
#include "world.h"

// Minecraft-specific geometry rules for the current full-cube world.
// No general-purpose physics engine, rigid bodies, or collider ownership.
struct WorldPhysics {
  explicit WorldPhysics(
    const World& world,
    const PlayerController &controller) : world(world), controller(controller) {}

  bool is_grounded(BoundingBox player) const;
  bool overlaps_block(BoundingBox player, Vector3Int cell, BlockType block_type) const;

  // Resolve the intended displacement one axis at a time, sliding
  // along walls and cancelling velocity on blocked axes.
  void move_player(
    Vector3& feet,
    Vector3& velocity,
    Vector3 size,
    Vector3 displacement
  ) const;

private:
  const World& world;
  const PlayerController& controller;
};
