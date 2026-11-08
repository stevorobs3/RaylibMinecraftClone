#pragma once

#include "raylib.h"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

class PlayerController;

class PhysicsWorld {
public:
  using Layer = std::uint16_t;
  using BodyId = std::uint32_t;
  using CollisionPair = std::pair<Layer, Layer>;

  enum class Motion {
    Static,
    Kinematic,
    Dynamic
  };

  static constexpr Layer NON_MOVING = 0;
  static constexpr Layer MOVING = 1;

  static constexpr std::array ALL_LAYERS{NON_MOVING, MOVING};
  static constexpr std::array ALLOWED_COLLISIONS{
    CollisionPair{NON_MOVING, MOVING},
    CollisionPair{MOVING, MOVING}
  };

  // A move-only owning handle. Destroying it removes its Jolt body.
  // PhysicsWorld must outlive every Cube it created.
  struct Cube {
    Cube(const Cube&) = delete;
    Cube& operator=(const Cube&) = delete;
    Cube(Cube&& other) noexcept;
    Cube& operator=(Cube&& other) noexcept;
    ~Cube();

    BodyId id() const;
    Vector3 position() const;
    void set_position(Vector3 position);
    Vector3 velocity() const;
    void set_velocity(Vector3 velocity);
    void add_impulse(Vector3 impulse);
    Vector3 size() const;

  private:
    friend class PhysicsWorld;
    Cube(PhysicsWorld* world, BodyId id, Vector3 size);
    void release() noexcept;

    PhysicsWorld* world_ = nullptr;
    BodyId id_ = 0;
    Vector3 size_{};
  };

  struct RayHit {
    BodyId body_id;
    Vector3 position;
    Vector3 normal;
    float distance;
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

  std::optional<RayHit> raycast(
    Vector3 origin,
    Vector3 direction,
    float reach
  ) const;

  void update(float dt);

private:
  friend class PlayerController;
  friend struct Cube;

  void remove_body(BodyId id) noexcept;

  struct Impl;
  std::unique_ptr<Impl> impl;
};

// Only physics-related player state lives here. Input polling, block editing
// and block IDs remain game code. Camera3D is owned here so looking and
// walking use the same orientation.
struct PlayerMovementInput {
  Vector2 movement{};  // x: forward/back, y: strafe right/left
  Vector2 look_delta{};
  bool jump = false;
};

class PlayerController {
public:
  PlayerController(
    PhysicsWorld& physics,
    Camera3D camera,
    Vector3 feet,
    Vector3 size,
    PhysicsWorld::Layer layer
  );
  ~PlayerController();

  PlayerController(const PlayerController&) = delete;
  PlayerController& operator=(const PlayerController&) = delete;

  void update(const PlayerMovementInput& input, float frame_dt);
  void reset(Vector3 feet);

  Vector3 position() const;  // Centre of feet
  Vector3 velocity() const;
  bool grounded() const;
  BoundingBox bounds() const;
  const Camera3D& camera() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl;
};
