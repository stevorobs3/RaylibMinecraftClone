#include "world_physics.h"

#include <algorithm>
#include <cmath>

namespace {

BoundingBox player_bounds(Vector3 feet, Vector3 size) {
  return {
    {feet.x - size.x * 0.5f, feet.y, feet.z - size.z * 0.5f},
    {feet.x + size.x * 0.5f, feet.y + size.y, feet.z + size.z * 0.5f}
  };
}

bool overlap(const BoundingBox& a, const BoundingBox& b) {
  return a.min.x < b.max.x && a.max.x > b.min.x &&
         a.min.y < b.max.y && a.max.y > b.min.y &&
         a.min.z < b.max.z && a.max.z > b.min.z;
}

float axis_min(const BoundingBox& box, int axis) {
  return axis == 0 ? box.min.x : axis == 1 ? box.min.y : box.min.z;
}

float axis_max(const BoundingBox& box, int axis) {
  return axis == 0 ? box.max.x : axis == 1 ? box.max.y : box.max.z;
}

float& axis_component(Vector3& value, int axis) {
  return axis == 0 ? value.x : axis == 1 ? value.y : value.z;
}

bool overlaps_other_axes(
  const BoundingBox& a, const BoundingBox& b, int axis
) {
  for (int other = 0; other < 3; other++) {
    if (other == axis)
      continue;
    if (axis_min(a, other) >= axis_max(b, other) ||
        axis_max(a, other) <= axis_min(b, other)) {
      return false;
    }
  }
  return true;
}

// Enumerate only cells intersecting the given half-open region.
template<typename Fn>
void for_cells(
  const World& world, const BoundingBox& region, Fn&& visit
) {
  const int min_x = static_cast<int>(std::floor(region.min.x));
  const int min_y = std::max(
    0, static_cast<int>(std::floor(region.min.y))
  );
  const int min_z = static_cast<int>(std::floor(region.min.z));
  const int max_x = static_cast<int>(std::ceil(region.max.x));
  const int max_y = std::min(
    World::height, static_cast<int>(std::ceil(region.max.y))
  );
  const int max_z = static_cast<int>(std::ceil(region.max.z));

  for (int x = min_x; x < max_x; x++)
    for (int y = min_y; y < max_y; y++)
      for (int z = min_z; z < max_z; z++) {
        const Vector3Int cell{x, y, z};
        if (world.try_get(cell).value_or(BlockType::AIR) != BlockType::AIR)
          visit(cell);
      }
}

} // namespace

bool WorldPhysics::player_overlaps_block(
  Vector3Int cell, BlockType type, const BoundingBox& player
) const {
  return type != BlockType::AIR &&
         overlap(player, Block::bounds(cell, type));
}

bool WorldPhysics::is_grounded(const BoundingBox& player) const {
  constexpr float vertical_tolerance = 0.04f;
  constexpr float edge_tolerance     = 0.002f;

  BoundingBox query = player;
  query.min.y       = player.min.y - vertical_tolerance;
  query.max.y       = player.min.y + vertical_tolerance;
  bool grounded     = false;
  for_cells(world, query, [&](Vector3Int cell) {
    const BoundingBox block = Block::bounds(cell, world.get(cell));
    if (std::abs(block.max.y - player.min.y) > vertical_tolerance)
      return;
    if (player.max.x <= block.min.x + edge_tolerance ||
        player.min.x >= block.max.x - edge_tolerance ||
        player.max.z <= block.min.z + edge_tolerance ||
        player.min.z >= block.max.z - edge_tolerance) {
      return;
    }
    grounded = true;
  });
  return grounded;
}

void WorldPhysics::move_player(
  Vector3& feet, Vector3& velocity, Vector3 size, Vector3 displacement
) const {
  // X, Z, Y: preserve sliding against a wall while applying gravity last.
  for (int axis : {0, 2, 1}) {
    const float delta = axis_component(displacement, axis);
    if (delta == 0.0f)
      continue;

    const BoundingBox start  = player_bounds(feet, size);
    Vector3 destination_feet = feet;
    axis_component(destination_feet, axis) += delta;
    const BoundingBox destination = player_bounds(destination_feet, size);

    const BoundingBox sweep{
      {
        std::min(start.min.x, destination.min.x),
        std::min(start.min.y, destination.min.y),
        std::min(start.min.z, destination.min.z)
      },
      {
        std::max(start.max.x, destination.max.x),
        std::max(start.max.y, destination.max.y),
        std::max(start.max.z, destination.max.z)
      }
    };

    float allowed = delta;
    for_cells(world, sweep, [&](Vector3Int cell) {
      const BoundingBox block = Block::bounds(cell, world.get(cell));
      if (!overlaps_other_axes(start, block, axis))
        return;

      if (delta > 0.0f &&
          axis_max(start, axis) <= axis_min(block, axis) &&
          axis_max(destination, axis) > axis_min(block, axis)) {
        allowed = std::min(
          allowed, axis_min(block, axis) - axis_max(start, axis)
        );
      } else if (delta < 0.0f &&
                 axis_min(start, axis) >= axis_max(block, axis) &&
                 axis_min(destination, axis) < axis_max(block, axis)) {
        allowed = std::max(
          allowed, axis_max(block, axis) - axis_min(start, axis)
        );
      }
    });

    axis_component(feet, axis) += allowed;
    if (allowed != delta)
      axis_component(velocity, axis) = 0.0f;
  }
}
