#include "player.h"

#include <raylib.h>
#include <cmath>

void Player::find_target(const World& world, const Camera3D& camera) {
  const Vector2 centre{
    GetScreenWidth() / 2.0f,
    GetScreenHeight() / 2.0f
  };
  find_target(world,GetScreenToWorldRay(centre,camera));
}
void Player::find_target(const World& world,const Ray& ray) {
  float nearest = 5.0f;
  target.reset();
  for (Vector3Int grid_position : world.positions_within_range(ray.position, static_cast<int>(nearest))) {
    BlockType block_type = world.get(grid_position);
    if (block_type == BlockType::AIR) {
      continue;
    }
    const BoundingBox bounds = Block::bounds(grid_position, block_type);
    const RayCollision hit = GetRayCollisionBox(ray, bounds);
    if (hit.hit && hit.distance < nearest) {
      nearest = hit.distance;
      target = BlockHit{
        .position = grid_position,
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
  BoundingBox player_bounds,
  BlockAction action
) {
  if (!target) {
    return;
  }

  const BlockHit hit = *target;
  if (action == BlockAction::Remove && world.get(hit.position) != BlockType::BEDROCK) {
    world.set(hit.position, BlockType::AIR);
    target.reset();
    return;
  }

  if (action == BlockAction::Place) {
    const Vector3Int cell = hit.position + hit.face_normal;
    if (!world.contains(cell)) {
      return;
    }

    BlockType block_type = world.get(cell);
    if (block_type == BlockType::AIR &&
        !physics.player_overlaps_block(
          cell,
          BlockType::DIRT,
          player_bounds)) {
      world.set(cell, BlockType::DIRT);
      target.reset();
    }
  }
}
