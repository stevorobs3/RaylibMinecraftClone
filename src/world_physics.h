#pragma once

#include "world.h"
#include "physics/physics.h"

#include <array>
#include <optional>
#include <unordered_map>

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
