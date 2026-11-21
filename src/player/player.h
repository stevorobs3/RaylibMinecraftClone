#pragma once
#include <optional>
#include <raylib.h>
#include "block_action.h"
#include "vector.h"
#include "world/world.h"
#include "world_physics.h"

struct BlockHit {
  Vector3Int position;
  Vector3Int face_normal;
};

struct Player {
  std::optional<BlockHit> target;
  void find_target(const World& world, const Camera3D& camera);
  // deterministic, headless
  void find_target(const World& world, const Ray& ray);
  void manipulate_blocks(World& world, const WorldPhysics& physics,
                         BoundingBox player_bounds, BlockAction action);
};
