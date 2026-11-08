
#include "physics.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <limits>
#include <thread>

namespace {

// --------------------------------------------------
// Jolt runtime
// --------------------------------------------------

class JoltRuntime {
public:
  JoltRuntime() {
    JPH::RegisterDefaultAllocator();

    JPH::Factory::sInstance =
      new JPH::Factory();

    JPH::RegisterTypes();
  }

  ~JoltRuntime() {
    JPH::UnregisterTypes();

    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
  }

  JoltRuntime(const JoltRuntime&) = delete;
  JoltRuntime& operator=(const JoltRuntime&) = delete;
};

JoltRuntime& get_jolt_runtime() {
  static JoltRuntime runtime;
  return runtime;
}

// --------------------------------------------------
// Layer configuration
// --------------------------------------------------

constexpr auto& layers =
  PhysicsWorld::ALL_LAYERS;

constexpr auto& collisions =
  PhysicsWorld::ALLOWED_COLLISIONS;

constexpr unsigned int layer_count =
  static_cast<unsigned int>(layers.size());

static_assert(layer_count > 0);

static_assert(
  layer_count <=
  std::numeric_limits<JPH::BroadPhaseLayer::Type>::max()
);

// Layer IDs must be consecutive, starting at zero.
constexpr bool valid_layers() {
  for (std::size_t i = 0; i < layers.size(); ++i) {
    if (layers[i] != i) {
      return false;
    }
  }

  return true;
}

static_assert(valid_layers());

constexpr bool valid_collisions() {
  for (auto [a, b] : collisions) {
    if (a >= layer_count || b >= layer_count) {
      return false;
    }
  }

  return true;
}

static_assert(valid_collisions());

// Collision rules are symmetric.
constexpr bool can_collide(
  PhysicsWorld::Layer a,
  PhysicsWorld::Layer b
) {
  for (auto [x, y] : collisions) {
    if ((a == x && b == y) ||
        (a == y && b == x)) {
      return true;
    }
  }

  return false;
}

// --------------------------------------------------
// Broad-phase layer mapping
// --------------------------------------------------

class BPLayerInterface final
  : public JPH::BroadPhaseLayerInterface {
public:
  JPH::uint GetNumBroadPhaseLayers()
    const override {
    return layer_count;
  }

  JPH::BroadPhaseLayer GetBroadPhaseLayer(
    JPH::ObjectLayer layer
  ) const override {
    assert(layer < layer_count);

    // One broad-phase layer per object layer.
    return JPH::BroadPhaseLayer{
      static_cast<JPH::BroadPhaseLayer::Type>(
        layer
      )
    };
  }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
  const char* GetBroadPhaseLayerName(
    JPH::BroadPhaseLayer
  ) const override {
    return "GameLayer";
  }
#endif
};

// --------------------------------------------------
// Object vs object filtering
// --------------------------------------------------

class ObjectLayerFilter final
  : public JPH::ObjectLayerPairFilter {
public:
  bool ShouldCollide(
    JPH::ObjectLayer a,
    JPH::ObjectLayer b
  ) const override {
    return can_collide(a, b);
  }
};

// --------------------------------------------------
// Object vs broad-phase filtering
// --------------------------------------------------

class BroadPhaseFilter final
  : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
  bool ShouldCollide(
    JPH::ObjectLayer object,
    JPH::BroadPhaseLayer broadphase
  ) const override {
    const auto other =
      static_cast<PhysicsWorld::Layer>(
        static_cast<JPH::BroadPhaseLayer::Type>(
          broadphase
        )
      );

    return can_collide(object, other);
  }
};

} // namespace

// --------------------------------------------------
// PhysicsWorld implementation
// --------------------------------------------------

struct PhysicsWorld::Impl {
  JPH::TempAllocatorImpl temp_allocator{
    10 * 1024 * 1024
  };

  JPH::JobSystemThreadPool job_system{
    JPH::cMaxPhysicsJobs,
    JPH::cMaxPhysicsBarriers,
    static_cast<int>(
      std::max(
        1u,
        std::thread::hardware_concurrency()
      ) - 1
    )
  };

  BPLayerInterface bp_layers;
  ObjectLayerFilter object_filter;
  BroadPhaseFilter broadphase_filter;

  JPH::PhysicsSystem system;

  Impl() {
    system.Init(
      10240,  // Maximum bodies
      0,      // Default body mutex count
      65536,  // Maximum body pairs
      10240,  // Maximum contact constraints
      bp_layers,
      broadphase_filter,
      object_filter
    );
  }
};

PhysicsWorld::PhysicsWorld()
  : impl(nullptr)
{
  // Initialise Jolt before constructing Impl.
  get_jolt_runtime();

  impl = std::make_unique<Impl>();
}

PhysicsWorld::~PhysicsWorld() = default;

void PhysicsWorld::update(float dt) {
  // Simulation stepping comes in a later lesson.
}
