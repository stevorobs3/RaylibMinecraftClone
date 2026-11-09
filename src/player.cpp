#include "player.h"

#include <raylib.h>
#include <cmath>

void Player::find_target(const World& world, const Camera3D& camera) {
  const Vector2 centre{
    GetScreenWidth() / 2.0f,
    GetScreenHeight() / 2.0f
  };
  const Ray ray = GetScreenToWorldRay(centre, camera);

  float nearest = 5.0f;
  target.reset();
  for (auto&& [block, x, y, z] : world.blocks_with_positions()) {
    if (block.block_type == BlockType::Air) {
      continue;
    }
    const BoundingBox bounds{
      .min = {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)},
      .max = {static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z + 1)}
    };
    const RayCollision hit = GetRayCollisionBox(ray, bounds);
    if (hit.hit && hit.distance < nearest) {
      nearest = hit.distance;
      target = BlockHit{
        .position = {x, y, z},
        .face_normal = {
          static_cast<int>(std::lround(hit.normal.x)),
          static_cast<int>(std::lround(hit.normal.y)),
          static_cast<int>(std::lround(hit.normal.z))
        }
      };
    }
  }
}

void Player::manipulate_blocks(
  World& world,
  const WorldPhysics& physics,
  const PlayerController& controller,
  const PlayerInput& input
) {
  if (!target) {
    return;
  }

  const BlockHit hit = *target;
  if (input.wants_remove_block()) {
    world.blocks[
      hit.position.x, hit.position.y, hit.position.z
    ].block_type = BlockType::Air;
    target.reset();
    return;
  }

  if (input.wants_place_block()) {
    const Vector3Int cell = hit.position + hit.face_normal;
    if (!world.contains(cell)) {
      return;
    }

    auto& block = world.blocks[cell.x, cell.y, cell.z];
    if (block.block_type == BlockType::Air &&
        !physics.overlaps_block(controller.bounds(), cell)) {
      block.block_type = BlockType::Dirt;
      target.reset();
    }
  }
}
