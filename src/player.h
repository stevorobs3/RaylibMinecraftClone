#pragma once

#include "physics/physics.h"
#include "world.h"
#include "world_physics.h"
#include "player_input.h"

#include <optional>

// Minecraft-specific player actions. Physics and camera live in
// PlayerController; this class only manages block interaction.
struct BlockHit {
  Vector3Int position;
  Vector3Int face_normal;
};

struct Player {
  std::optional<BlockHit> target;

  void find_target(
    const PhysicsWorld& physics,
    const WorldPhysics& colliders,
    const Camera3D& camera
  );

  void manipulate_blocks(
    World& world,
    WorldPhysics& colliders,
    const PlayerInput& input,
    const PlayerController& controller
  );
};
