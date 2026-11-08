
#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <utility>

class PhysicsWorld {
public:
  using Layer = std::uint16_t;
  using CollisionPair = std::pair<Layer, Layer>;

  static constexpr Layer NON_MOVING = 0;
  static constexpr Layer MOVING = 1;

  static constexpr std::array ALL_LAYERS{
    NON_MOVING,
    MOVING
  };

  static constexpr std::array ALLOWED_COLLISIONS{
    CollisionPair{NON_MOVING, MOVING},
    CollisionPair{MOVING, MOVING}
  };

  PhysicsWorld();
  ~PhysicsWorld();

  PhysicsWorld(const PhysicsWorld&) = delete;
  PhysicsWorld& operator=(const PhysicsWorld&) = delete;

  void update(float dt);

private:
  struct Impl;
  std::unique_ptr<Impl> impl;
};
