#include "player.h"

#include <raylib.h>
#include <cmath>

void Player::find_target(
  const PhysicsWorld& physics,
  const WorldPhysics& colliders,
  const Camera3D& camera
) {
  const Vector2 centre{
    GetScreenWidth() / 2.0f,
    GetScreenHeight() / 2.0f
  };
  const Ray ray = GetScreenToWorldRay(centre, camera);
  const auto hit = physics.raycast(ray.position, ray.direction, 5.0f);
  target.reset();

  if (!hit) {
    return;
  }

  const auto block = colliders.block_for(hit->body_id);
  if (!block) {
    return;
  }

  target = BlockHit{
    .position = *block,
    .face_normal = Vector3Int{
      static_cast<int>(std::lround(hit->normal.x)),
      static_cast<int>(std::lround(hit->normal.y)),
      static_cast<int>(std::lround(hit->normal.z))
    }
  };
}

void Player::manipulate_blocks(
  World& world,
  WorldPhysics& colliders,
  const PlayerInput& input,
  const PlayerController& controller
) {
  if (!target) {
    return;
  }

  BlockHit hit = *target;
  if (input.wants_remove_block()) {
    world.blocks[
      hit.position.x, hit.position.y, hit.position.z
    ].block_type = BlockType::Air;
    colliders.remove_block(hit.position);
    target.reset();
    return;
  }

  if (input.wants_place_block()) {
    Vector3Int position = hit.position + hit.face_normal;
    if (world.contains(position)) {
      auto& block = world.blocks[position.x, position.y, position.z];
      if (block.block_type == BlockType::Air) {
        // Reject placement if the new Jolt cube intersects the
        // virtual character. Don't put the player inside solid terrain.
        Vector3 centre{
          position.x + 0.5f,
          position.y + 0.5f,
          position.z + 0.5f
        };
        if (!controller.overlaps_cube(centre, {1.0f, 1.0f, 1.0f})) {
          colliders.add_block(position);
          block.block_type = BlockType::Dirt;
          target.reset();
        }
      }
    }
  }
}
