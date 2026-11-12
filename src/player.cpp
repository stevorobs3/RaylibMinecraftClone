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
  for (Vector3Int grid_position : world.positions_within_range(camera.position, nearest)) {
    BlockType block_type = world.get(grid_position);
    if (block_type == BlockType::Air) {
      continue;
    }
    const BoundingBox bounds = Block::bounds(block_type, grid_position);
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
  const PlayerController& controller,
  const PlayerInput& input
) {
  if (!target) {
    return;
  }

  const BlockHit hit = *target;
  if (input.wants_remove_block()) {
    world.set(hit.position.x, hit.position.y, hit.position.z, BlockType::Air);
    target.reset();
    return;
  }

  if (input.wants_place_block()) {
    const Vector3Int cell = hit.position + hit.face_normal;
    if (!world.contains(cell)) {
      return;
    }

    BlockType block_type = world.get(cell.x, cell.y, cell.z);
    if (block_type == BlockType::Air &&
        !physics.overlaps_block(controller.bounds(), cell)) {
      world.set(cell.x, cell.y, cell.z, BlockType::Dirt);
      target.reset();
    }
  }
}
