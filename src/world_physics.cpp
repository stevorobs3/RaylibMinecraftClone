#include "world_physics.h"
#include "player_controller.h"

#include <algorithm>
#include <cmath>

bool overlap(BoundingBox a, BoundingBox b) {
  return a.min.x < b.max.x && a.max.x > b.min.x &&
         a.min.y < b.max.y && a.max.y > b.min.y &&
         a.min.z < b.max.z && a.max.z > b.min.z;
}


bool WorldPhysics::overlaps_block(
  BoundingBox player,
  Vector3Int cell,
  BlockType block_type
) const {
  return overlap(player, Block::bounds(cell, block_type));
}

bool WorldPhysics::is_grounded(BoundingBox player) const {
  // Accept small numerical gaps, but only exposed block TOP faces.
  constexpr float height_tolerance = 0.04f;
  constexpr float edge_tolerance = 0.002f;

  //todo: base this off off collisions and how it changes the y position
  for (Vector3Int block_pos : controller.blocks_overlapping_with_player()) {
    BlockType block_type = world.get(block_pos);
    if (block_type == BlockType::Air) {
      continue;
    }

    // The top of a block buried beneath another full cube isn't a
    // walkable surface. This also avoids false jumps at wall seams.
    if (block_pos.y + 1 < World::height &&
        world.get(block_pos.x, block_pos.y + 1, block_pos.z) != BlockType::Air) {
      continue;
    }

    const float top = static_cast<float>(block_pos.y + 1);
    if (std::abs(player.min.y - top) > height_tolerance) {
      continue;
    }

    const bool overlaps_x =
      player.max.x > static_cast<float>(block_pos.x) + edge_tolerance &&
      player.min.x < static_cast<float>(block_pos.x + 1) - edge_tolerance;
    const bool overlaps_z =
      player.max.z > static_cast<float>(block_pos.z) + edge_tolerance &&
      player.min.z < static_cast<float>(block_pos.z + 1) - edge_tolerance;

    if (overlaps_x && overlaps_z) {
      return true;
    }
  }

  return false;
}

void WorldPhysics::move_player(
  Vector3& feet,
  Vector3& velocity,
  Vector3 size,
  Vector3 displacement
) const {
  // Restore the original axis-separated AABB collision approach:
  // horizontal motion first, then vertical falling / jumping.
  // Each axis is resolved against the current world before the next.
  BoundingBox world_bb = world.bounds();
  auto resolve_axis = [&](char axis, float delta) {
    if (delta == 0.0f) {
      return;
    }

    float* coordinate = axis == 'x' ? &feet.x :
                        axis == 'y' ? &feet.y : &feet.z;
    *coordinate += delta;

    // todo: moving in negative x on bottom of world teleports the player 
    // up to the next block's y position? why??? strange indeed
    if (axis == 'x') {
      feet.x = delta > 0.0f
        ? std::min(feet.x, (world_bb.max.x - size.x * 0.5f))
        : std::max(feet.x, (world_bb.min.x + size.x * 0.5f));
    }
    else if (axis == 'y') {
      feet.y = delta > 0.0f
        ? std::min(feet.y, (world_bb.max.y - size.y))
        : std::max(feet.y, world_bb.min.y);
      // if we've hit the ceiling, stop moving up
      if (feet.y == world_bb.max.y)
        velocity.y = 0.0f;
    }
    else {
      feet.z = delta > 0.0f
        ? std::min(feet.z, (world_bb.max.z - size.z * 0.5f))
        : std::max(feet.z, (world_bb.min.z + size.z * 0.5f));
    }

    for (Vector3Int block_pos : controller.blocks_overlapping_with_player()) {
      BlockType block_type = world.get(block_pos);
      if (block_type == BlockType::Air ||
          !overlap(controller.bounds(), Block::bounds(block_pos, block_type))) {
        continue;
      }

      if (axis == 'x') {
        feet.x = delta > 0.0f
          ? std::min(feet.x, static_cast<float>(block_pos.x) - size.x * 0.5f)
          : std::max(feet.x, static_cast<float>(block_pos.x + 1) + size.x * 0.5f);
        velocity.x = 0.0f;
      } else if (axis == 'z') {
        feet.z = delta > 0.0f
          ? std::min(feet.z, static_cast<float>(block_pos.z) - size.z * 0.5f)
          : std::max(feet.z, static_cast<float>(block_pos.z + 1) + size.z * 0.5f);
        velocity.z = 0.0f;
      } else {
        // Player position is at the feet, not at the body's centre.
        feet.y = delta > 0.0f
          ? std::min(feet.y, static_cast<float>(block_pos.y) - size.y)
          : std::max(feet.y, static_cast<float>(block_pos.y + 1));
        velocity.y = 0.0f;
      }
    }
  };

  resolve_axis('x', displacement.x);
  resolve_axis('z', displacement.z);
  resolve_axis('y', displacement.y);
}
