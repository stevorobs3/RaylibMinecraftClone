
#pragma once

#include "raylib.h"

#include <array>
#include <cstdint>
#include <memory>
#include <utility>

class PhysicsWorld {
public:
  using Layer = std::uint16_t;
  using CollisionPair = std::pair<Layer, Layer>;

  enum class Motion {
    Static,
    Kinematic,
    Dynamic
  };

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

  struct Cube {
    Vector3 position() const;
    void set_position(Vector3 position);

    Vector3 velocity() const;
    void set_velocity(Vector3 velocity);

    void add_impulse(Vector3 impulse);

    Vector3 size() const;

  private:
    friend class PhysicsWorld;

    Cube(
      PhysicsWorld* world,
      std::uint32_t id,
      Vector3 size
    );

    PhysicsWorld* world_;
    std::uint32_t id_;
    Vector3 size_;
  };

  PhysicsWorld();
  ~PhysicsWorld();

  PhysicsWorld(const PhysicsWorld&) = delete;
  PhysicsWorld& operator=(const PhysicsWorld&) = delete;

  Cube add_cube(
    Vector3 position,
    Vector3 size,
    Layer layer,
    Motion motion = Motion::Dynamic
  );

  void update(float dt);

private:
  struct Impl;
  std::unique_ptr<Impl> impl;
};
