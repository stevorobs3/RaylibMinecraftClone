#pragma once

#include <optional>

#include <raylib.h>

#include "player_input.h"
#include "player/player_controller.h"
#include "vector.h"
#include "world/world.h"
#include "world_physics.h"

struct BlockHit {
  Vector3Int position;
  Vector3Int face_normal;
};

// Game-specific block interactions; camera and movement live in PlayerController.
struct Player {
  std::optional<BlockHit> target;

  void find_target(const World& world, const Camera3D& camera);
  void manipulate_blocks(
    World& world,
    const WorldPhysics& physics,
    const PlayerController& controller,
    const PlayerInput& input
  );
};
