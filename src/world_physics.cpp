#include "world_physics.h"

#include <algorithm>
#include <cmath>

namespace {

BoundingBox player_bounds(Vector3 feet, Vector3 size) {
  return {
    .min = {feet.x - size.x * 0.5f, feet.y, feet.z - size.z * 0.5f},
    .max = {feet.x + size.x * 0.5f, feet.y + size.y, feet.z + size.z * 0.5f}
  };
}

BoundingBox block_bounds(int x, int y, int z) {
  return {
    .min = {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)},
    .max = {
      static_cast<float>(x + 1),
      static_cast<float>(y + 1),
      static_cast<float>(z + 1)
    }
  };
}

bool overlap(BoundingBox a, BoundingBox b) {
  return a.min.x < b.max.x && a.max.x > b.min.x &&
         a.min.y < b.max.y && a.max.y > b.min.y &&
         a.min.z < b.max.z && a.max.z > b.min.z;
}

} // namespace

bool WorldPhysics::overlaps_block(BoundingBox player, Vector3Int cell) const {
  return overlap(player, block_bounds(cell.x, cell.y, cell.z));
}

bool WorldPhysics::is_grounded(BoundingBox player) const {
  // With our own collision resolution, feet snap exactly to block tops.
  // Allow only floating-point noise, not Jolt's former 4 cm padding.
  constexpr float height_tolerance = 0.002f;
  constexpr float edge_tolerance = 0.002f;

  for (auto&& [block, x, y, z] : world_.blocks_with_positions()) {
    if (block.block_type == BlockType::Air) {
      continue;
    }

    // The top of a block buried beneath another full cube isn't a
    // walkable surface. This also avoids false jumps at wall seams.
    if (y + 1 < World::height &&
        world_.blocks[x, y + 1, z].block_type != BlockType::Air) {
      continue;
    }

    const float top = static_cast<float>(y + 1);
    if (std::abs(player.min.y - top) > height_tolerance) {
      continue;
    }

    const bool overlaps_x =
      player.max.x > static_cast<float>(x) + edge_tolerance &&
      player.min.x < static_cast<float>(x + 1) - edge_tolerance;
    const bool overlaps_z =
      player.max.z > static_cast<float>(z) + edge_tolerance &&
      player.min.z < static_cast<float>(z + 1) - edge_tolerance;

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
  auto resolve_axis = [&](char axis, float delta) {
    if (delta == 0.0f) {
      return;
    }

    float* coordinate = axis == 'x' ? &feet.x :
                        axis == 'y' ? &feet.y : &feet.z;
    *coordinate += delta;

    for (auto&& [block, x, y, z] : world_.blocks_with_positions()) {
      if (block.block_type == BlockType::Air ||
          !overlap(player_bounds(feet, size), block_bounds(x, y, z))) {
        continue;
      }

      if (axis == 'x') {
        feet.x = delta > 0.0f
          ? std::min(feet.x, static_cast<float>(x) - size.x * 0.5f)
          : std::max(feet.x, static_cast<float>(x + 1) + size.x * 0.5f);
        velocity.x = 0.0f;
      } else if (axis == 'z') {
        feet.z = delta > 0.0f
          ? std::min(feet.z, static_cast<float>(z) - size.z * 0.5f)
          : std::max(feet.z, static_cast<float>(z + 1) + size.z * 0.5f);
        velocity.z = 0.0f;
      } else {
        // Player position is at the feet, not at the body's centre.
        feet.y = delta > 0.0f
          ? std::min(feet.y, static_cast<float>(y) - size.y)
          : std::max(feet.y, static_cast<float>(y + 1));
        velocity.y = 0.0f;
      }
    }
  };

  resolve_axis('x', displacement.x);
  resolve_axis('z', displacement.z);
  resolve_axis('y', displacement.y);
}
