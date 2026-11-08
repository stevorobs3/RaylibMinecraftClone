
#include "physics.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <thread>

namespace {

// --------------------------------------------------
// Jolt runtime
// --------------------------------------------------

class JoltRuntime {
public:
  JoltRuntime() {
    JPH::RegisterDefaultAllocator();

    JPH::Factory::sInstance = new JPH::Factory();
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
// Collision layers
// --------------------------------------------------

constexpr auto& layers = PhysicsWorld::ALL_LAYERS;
constexpr auto& collisions = PhysicsWorld::ALLOWED_COLLISIONS;

constexpr unsigned int layer_count =
  static_cast<unsigned int>(layers.size());

static_assert(layer_count > 0);
static_assert(
  layer_count <=
  std::numeric_limits<JPH::BroadPhaseLayer::Type>::max()
);

constexpr bool valid_layers() {
  for (std::size_t i = 0; i < layers.size(); ++i) {
    if (layers[i] != i) {
      return false;
    }
  }

  return true;
}

constexpr bool valid_collisions() {
  for (auto [a, b] : collisions) {
    if (a >= layer_count || b >= layer_count) {
      return false;
    }
  }

  return true;
}

static_assert(valid_layers());
static_assert(valid_collisions());

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
// Jolt collision filters
// --------------------------------------------------

class BPLayerInterface final
  : public JPH::BroadPhaseLayerInterface {
public:
  JPH::uint GetNumBroadPhaseLayers() const override {
    return layer_count;
  }

  JPH::BroadPhaseLayer GetBroadPhaseLayer(
    JPH::ObjectLayer layer
  ) const override {
    assert(layer < layer_count);

    return JPH::BroadPhaseLayer{
      static_cast<JPH::BroadPhaseLayer::Type>(layer)
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

JPH::EMotionType to_jolt_motion(
  PhysicsWorld::Motion motion
) {
  switch (motion) {
    case PhysicsWorld::Motion::Static:
      return JPH::EMotionType::Static;

    case PhysicsWorld::Motion::Kinematic:
      return JPH::EMotionType::Kinematic;

    case PhysicsWorld::Motion::Dynamic:
      return JPH::EMotionType::Dynamic;
  }

  throw std::invalid_argument("Invalid motion type");
}

JPH::Vec3 to_jolt_vec(Vector3 v) {
  return JPH::Vec3{v.x, v.y, v.z};
}

Vector3 from_jolt_vec(JPH::RVec3Arg v) {
  return {
    static_cast<float>(v.GetX()),
    static_cast<float>(v.GetY()),
    static_cast<float>(v.GetZ())
  };
}

JPH::BodyID to_body_id(std::uint32_t id) {
  return JPH::BodyID{id};
}

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
      std::max(1u, std::thread::hardware_concurrency()) - 1
    )
  };

  BPLayerInterface bp_layers;
  ObjectLayerFilter object_filter;
  BroadPhaseFilter broadphase_filter;

  JPH::PhysicsSystem system;

  Impl() {
    system.Init(
      10240,
      0,
      65536,
      10240,
      bp_layers,
      broadphase_filter,
      object_filter
    );
  }

  ~Impl() {
    auto& bodies = system.GetBodyInterface();
    system.OptimizeBroadPhase();

    // Bodies are cleaned up when PhysicsSystem is destroyed.
  }
};

PhysicsWorld::PhysicsWorld()
  : impl(nullptr)
{
  get_jolt_runtime();
  impl = std::make_unique<Impl>();
}

PhysicsWorld::~PhysicsWorld() = default;

// --------------------------------------------------
// Cube creation
// --------------------------------------------------

PhysicsWorld::Cube PhysicsWorld::add_cube(
  Vector3 position,
  Vector3 size,
  Layer layer,
  Motion motion
) {
  if (layer >= layer_count) {
    throw std::invalid_argument("Invalid physics layer");
  }

  if (size.x <= 0 || size.y <= 0 || size.z <= 0) {
    throw std::invalid_argument("Cube size must be positive");
  }

  auto& bodies = impl->system.GetBodyInterface();

  JPH::BoxShapeSettings shape_settings{
    to_jolt_vec({
      size.x * 0.5f,
      size.y * 0.5f,
      size.z * 0.5f
    })
  };

  auto shape_result = shape_settings.Create();

  if (shape_result.HasError()) {
    throw std::runtime_error(
      shape_result.GetError().c_str()
    );
  }

  JPH::BodyCreationSettings settings{
    shape_result.Get(),
    JPH::RVec3{
      position.x,
      position.y,
      position.z
    },
    JPH::Quat::sIdentity(),
    to_jolt_motion(motion),
    layer
  };

  const auto activation =
    motion == Motion::Static
      ? JPH::EActivation::DontActivate
      : JPH::EActivation::Activate;

  JPH::BodyID id = bodies.CreateAndAddBody(
    settings,
    activation
  );

  if (id.IsInvalid()) {
    throw std::runtime_error("Failed to create cube");
  }

  return Cube{
    this,
    id.GetIndexAndSequenceNumber(),
    size
  };
}

// --------------------------------------------------
// Cube interface
// --------------------------------------------------

PhysicsWorld::Cube::Cube(
  PhysicsWorld* world,
  std::uint32_t id,
  Vector3 size
)
  : world_(world),
    id_(id),
    size_(size)
{}

Vector3 PhysicsWorld::Cube::position() const {
  auto& bodies = world_->impl->system.GetBodyInterface();

  return from_jolt_vec(
    bodies.GetCenterOfMassPosition(to_body_id(id_))
  );
}

void PhysicsWorld::Cube::set_position(
  Vector3 position
) {
  auto& bodies = world_->impl->system.GetBodyInterface();

  bodies.SetPosition(
    to_body_id(id_),
    JPH::RVec3{
      position.x,
      position.y,
      position.z
    },
    JPH::EActivation::Activate
  );
}

Vector3 PhysicsWorld::Cube::velocity() const {
  auto& bodies = world_->impl->system.GetBodyInterface();

  return from_jolt_vec(
    bodies.GetLinearVelocity(to_body_id(id_))
  );
}

void PhysicsWorld::Cube::set_velocity(
  Vector3 velocity
) {
  auto& bodies = world_->impl->system.GetBodyInterface();

  bodies.SetLinearVelocity(
    to_body_id(id_),
    to_jolt_vec(velocity)
  );
}

void PhysicsWorld::Cube::add_impulse(
  Vector3 impulse
) {
  auto& bodies = world_->impl->system.GetBodyInterface();

  bodies.AddImpulse(
    to_body_id(id_),
    to_jolt_vec(impulse)
  );
}

Vector3 PhysicsWorld::Cube::size() const {
  return size_;
}

// --------------------------------------------------
// Simulation
// --------------------------------------------------

void PhysicsWorld::update(float dt) {
  // We'll implement stepping in the next lesson.
}
