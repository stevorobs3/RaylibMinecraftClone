#pragma once

#include "world.h"
#include "physics/physics.h"

#include <array>
#include <cmath>
#include <optional>
#include <unordered_map>
#include <utility>

// Game-side adapter: the physics library knows only cubes, not voxels.
class WorldPhysics {
public:
  WorldPhysics(PhysicsWorld& physics, PhysicsWorld::Layer layer)
    : physics_(physics), layer_(layer) {}

  void rebuild(const World& world) {
    for (auto& collider : colliders_) {
      collider.reset();
    }
    by_body_.clear();

    for (auto&& [block, x, y, z] : world.blocks_with_positions()) {
      if (block.block_type != BlockType::Air) {
        add_block({x, y, z});
      }
    }
  }

  void add_block(Vector3Int position) {
    const auto index = offset(position);
    if (colliders_[index]) {
      return;
    }

    Vector3 centre{
      position.x + 0.5f,
      position.y + 0.5f,
      position.z + 0.5f
    };
    auto cube = physics_.add_cube(
      centre, {1.0f, 1.0f, 1.0f},
      layer_, PhysicsWorld::Motion::Static
    );
    const auto id = cube.id();
    by_body_.emplace(id, position);
    colliders_[index].emplace(std::move(cube));
  }

  void remove_block(Vector3Int position) {
    auto& collider = colliders_[offset(position)];
    if (collider) {
      by_body_.erase(collider->id());
      collider.reset();
    }
  }

  // Gameplay support rule for the current full-cube voxel world.
  // Only a block's TOP face can support the player's feet.
  bool is_grounded(const World& world, BoundingBox player) const {
    constexpr float height_tolerance = 0.04f; // Jolt contact padding
    constexpr float edge_tolerance = 0.002f;  // Ignore merely touching edges

    for (auto&& [block, x, y, z] : world.blocks_with_positions()) {
      if (block.block_type == BlockType::Air) {
        continue;
      }

      const float top = static_cast<float>(y) + 1.0f;
      if (std::abs(player.min.y - top) > height_tolerance) {
        continue;
      }

      const bool overlaps_x =
        player.max.x > static_cast<float>(x) + edge_tolerance &&
        player.min.x < static_cast<float>(x) + 1.0f - edge_tolerance;
      const bool overlaps_z =
        player.max.z > static_cast<float>(z) + edge_tolerance &&
        player.min.z < static_cast<float>(z) + 1.0f - edge_tolerance;

      if (overlaps_x && overlaps_z) {
        return true;
      }
    }
    return false;
  }

  std::optional<Vector3Int> block_for(PhysicsWorld::BodyId id) const {
    auto found = by_body_.find(id);
    if (found == by_body_.end()) {
      return std::nullopt;
    }
    return found->second;
  }

private:
  static std::size_t offset(Vector3Int position) {
    return static_cast<std::size_t>(
      (position.x * World::height + position.y) * World::n + position.z
    );
  }

  PhysicsWorld& physics_;
  PhysicsWorld::Layer layer_;
  std::array<std::optional<PhysicsWorld::Cube>, World::blockCount> colliders_{};
  std::unordered_map<PhysicsWorld::BodyId, Vector3Int> by_body_;
};
