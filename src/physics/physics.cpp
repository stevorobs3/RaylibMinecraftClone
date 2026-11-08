
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
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Body/BodyFilter.h>

#include <algorithm>
#include <cmath>
#include <utility>
#include <unordered_set>
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
  static_cast<unsigned int>(std::numeric_limits<JPH::BroadPhaseLayer::Type>::max()) + 1u
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

template<class Vec>
Vector3 from_jolt_vec(const Vec& v) {
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


class CharacterLayerFilter final : public JPH::ObjectLayerFilter {
public:
  explicit CharacterLayerFilter(PhysicsWorld::Layer layer) : layer_(layer) {}

  bool ShouldCollide(JPH::ObjectLayer other) const override {
    return can_collide(layer_, other);
  }

private:
  PhysicsWorld::Layer layer_;
};

struct PhysicsWorld::Impl {
  JPH::TempAllocatorImpl temp_allocator{10 * 1024 * 1024};
  JPH::JobSystemThreadPool job_system{
    JPH::cMaxPhysicsJobs,
    JPH::cMaxPhysicsBarriers,
    static_cast<int>(std::max(1u, std::thread::hardware_concurrency()) - 1)
  };

  BPLayerInterface bp_layers;
  ObjectLayerFilter object_filter;
  BroadPhaseFilter broadphase_filter;
  JPH::PhysicsSystem system;
  std::unordered_set<BodyId> bodies;

  Impl() {
    system.Init(
      10240, 0, 65536, 10240,
      bp_layers, broadphase_filter, object_filter
    );
  }

  ~Impl() {
    auto& interface = system.GetBodyInterface();
    for (BodyId id : bodies) {
      JPH::BodyID body_id = to_body_id(id);
      interface.RemoveBody(body_id);
      interface.DestroyBody(body_id);
    }
  }
};

PhysicsWorld::PhysicsWorld() : impl(nullptr) {
  get_jolt_runtime();
  impl = std::make_unique<Impl>();
}

PhysicsWorld::~PhysicsWorld() = default;

PhysicsWorld::Cube PhysicsWorld::add_cube(
  Vector3 position,
  Vector3 size,
  Layer layer,
  Motion motion
) {
  if (layer >= layer_count) {
    throw std::invalid_argument("Invalid physics layer");
  }
  if (size.x <= 0.0f || size.y <= 0.0f || size.z <= 0.0f) {
    throw std::invalid_argument("Cube size must be positive");
  }

  // No rounded corners: voxel faces should meet exactly.
  JPH::BoxShapeSettings shape_settings{
    to_jolt_vec({
      size.x * 0.5f,
      size.y * 0.5f,
      size.z * 0.5f
    }),
    0.0f
  };
  auto shape = shape_settings.Create();
  if (shape.HasError()) {
    throw std::runtime_error(shape.GetError().c_str());
  }

  JPH::BodyCreationSettings settings{
    shape.Get(),
    JPH::RVec3{position.x, position.y, position.z},
    JPH::Quat::sIdentity(),
    to_jolt_motion(motion),
    layer
  };

  JPH::BodyID id = impl->system.GetBodyInterface().CreateAndAddBody(
    settings,
    motion == Motion::Static
      ? JPH::EActivation::DontActivate
      : JPH::EActivation::Activate
  );
  if (id.IsInvalid()) {
    throw std::runtime_error("Failed to create physics cube");
  }

  BodyId packed = id.GetIndexAndSequenceNumber();
  try {
    impl->bodies.insert(packed);
  } catch (...) {
    auto& bodies = impl->system.GetBodyInterface();
    bodies.RemoveBody(id);
    bodies.DestroyBody(id);
    throw;
  }
  return Cube{this, packed, size};
}

void PhysicsWorld::remove_body(BodyId id) noexcept {
  if (!impl || impl->bodies.erase(id) == 0) {
    return;
  }

  JPH::BodyID body_id = to_body_id(id);
  auto& bodies = impl->system.GetBodyInterface();
  bodies.RemoveBody(body_id);
  bodies.DestroyBody(body_id);
}

PhysicsWorld::Cube::Cube(PhysicsWorld* world, BodyId id, Vector3 size)
  : world_(world), id_(id), size_(size) {}

PhysicsWorld::Cube::Cube(Cube&& other) noexcept
  : world_(std::exchange(other.world_, nullptr)),
    id_(std::exchange(other.id_, 0)),
    size_(other.size_) {}

PhysicsWorld::Cube& PhysicsWorld::Cube::operator=(Cube&& other) noexcept {
  if (this != &other) {
    release();
    world_ = std::exchange(other.world_, nullptr);
    id_ = std::exchange(other.id_, 0);
    size_ = other.size_;
  }
  return *this;
}

PhysicsWorld::Cube::~Cube() {
  release();
}

void PhysicsWorld::Cube::release() noexcept {
  if (world_ != nullptr) {
    world_->remove_body(id_);
    world_ = nullptr;
    id_ = 0;
  }
}

PhysicsWorld::BodyId PhysicsWorld::Cube::id() const {
  return id_;
}

Vector3 PhysicsWorld::Cube::position() const {
  return from_jolt_vec(
    world_->impl->system.GetBodyInterface().GetCenterOfMassPosition(to_body_id(id_))
  );
}

void PhysicsWorld::Cube::set_position(Vector3 position) {
  world_->impl->system.GetBodyInterface().SetPosition(
    to_body_id(id_),
    JPH::RVec3{position.x, position.y, position.z},
    JPH::EActivation::Activate
  );
}

Vector3 PhysicsWorld::Cube::velocity() const {
  return from_jolt_vec(
    world_->impl->system.GetBodyInterface().GetLinearVelocity(to_body_id(id_))
  );
}

void PhysicsWorld::Cube::set_velocity(Vector3 velocity) {
  world_->impl->system.GetBodyInterface().SetLinearVelocity(
    to_body_id(id_), to_jolt_vec(velocity)
  );
}

void PhysicsWorld::Cube::add_impulse(Vector3 impulse) {
  world_->impl->system.GetBodyInterface().AddImpulse(
    to_body_id(id_), to_jolt_vec(impulse)
  );
}

Vector3 PhysicsWorld::Cube::size() const {
  return size_;
}

std::optional<PhysicsWorld::RayHit> PhysicsWorld::raycast(
  Vector3 origin, Vector3 direction, float reach
) const {
  float length = std::sqrt(
    direction.x * direction.x +
    direction.y * direction.y +
    direction.z * direction.z
  );
  if (reach <= 0.0f || length == 0.0f) {
    return std::nullopt;
  }
  Vector3 displacement{
    direction.x * reach / length,
    direction.y * reach / length,
    direction.z * reach / length
  };

  JPH::RRayCast ray{
    JPH::RVec3{origin.x, origin.y, origin.z},
    to_jolt_vec(displacement)
  };
  JPH::RayCastResult hit;
  if (!impl->system.GetNarrowPhaseQuery().CastRay(ray, hit)) {
    return std::nullopt;
  }

  JPH::BodyLockRead lock{impl->system.GetBodyLockInterface(), hit.mBodyID};
  if (!lock.Succeeded()) {
    return std::nullopt;
  }

  auto point = ray.GetPointOnRay(hit.mFraction);
  auto normal = lock.GetBody().GetWorldSpaceSurfaceNormal(
    hit.mSubShapeID2, point
  );

  return RayHit{
    .body_id = hit.mBodyID.GetIndexAndSequenceNumber(),
    .position = from_jolt_vec(point),
    .normal = from_jolt_vec(normal),
    .distance = hit.mFraction * reach
  };
}

void PhysicsWorld::update(float dt) {
  if (dt > 0.0f) {
    impl->system.Update(
      dt, 1, &impl->temp_allocator, &impl->job_system
    );
  }
}

// --------------------------------------------------
// Jolt virtual character and first-person camera
// --------------------------------------------------

struct PlayerController::Impl {
  PhysicsWorld* world = nullptr;
  JPH::Ref<JPH::CharacterVirtual> character;
  PhysicsWorld::Layer layer = 0;
  Vector3 size{};
  Camera3D camera{};
  float yaw = 0.0f;
  float pitch = 0.0f;
};

PlayerController::PlayerController(
  PhysicsWorld& physics,
  Camera3D camera,
  Vector3 feet,
  Vector3 size,
  PhysicsWorld::Layer layer
) : impl(std::make_unique<Impl>()) {
  if (layer >= layer_count ||
      size.x <= 0.0f || size.y <= 0.0f || size.z <= 0.0f) {
    throw std::invalid_argument("Invalid player size or layer");
  }

  impl->world = &physics;
  impl->layer = layer;
  impl->size = size;
  impl->camera = camera;

  Vector3 forward{
    camera.target.x - camera.position.x,
    camera.target.y - camera.position.y,
    camera.target.z - camera.position.z
  };
  float length = std::sqrt(
    forward.x * forward.x + forward.y * forward.y + forward.z * forward.z
  );
  if (length > 0.0f) {
    impl->yaw = std::atan2(forward.z, forward.x);
    impl->pitch = std::asin(std::clamp(forward.y / length, -1.0f, 1.0f));
  }

  JPH::BoxShapeSettings box{
    JPH::Vec3{size.x * 0.5f, size.y * 0.5f, size.z * 0.5f},
    0.0f
  };
  auto shape = box.Create();
  if (shape.HasError()) {
    throw std::runtime_error(shape.GetError().c_str());
  }

  JPH::CharacterVirtualSettings settings;
  settings.mShape = shape.Get();
  settings.mShapeOffset = JPH::Vec3{0.0f, size.y * 0.5f, 0.0f};
  settings.mMaxSlopeAngle = JPH::DegreesToRadians(50.0f);

  impl->character = new JPH::CharacterVirtual(
    &settings,
    JPH::RVec3{feet.x, feet.y, feet.z},
    JPH::Quat::sIdentity(),
    &physics.impl->system
  );

  reset(feet);
}

PlayerController::~PlayerController() = default;

void PlayerController::reset(Vector3 feet) {
  auto& character = *impl->character;
  character.SetPosition(JPH::RVec3{feet.x, feet.y, feet.z});
  character.SetLinearVelocity(JPH::Vec3::sZero());

  CharacterLayerFilter filter{impl->layer};
  character.RefreshContacts(
    JPH::BroadPhaseLayerFilter{}, filter,
    JPH::BodyFilter{}, JPH::ShapeFilter{},
    impl->world->impl->temp_allocator
  );

  // Keep pitch and yaw through reset, as in the original game.
  const float cp = std::cos(impl->pitch);
  impl->camera.position = {
    feet.x, feet.y + 1.62f, feet.z
  };
  impl->camera.target = {
    feet.x + cp * std::cos(impl->yaw),
    feet.y + 1.62f + std::sin(impl->pitch),
    feet.z + cp * std::sin(impl->yaw)
  };
}

void PlayerController::update(
  const PlayerMovementInput& input, float frame_dt
) {
  constexpr float sensitivity = 0.002f;
  constexpr float move_speed = 4.0f;
  constexpr float gravity = -9.81f;
  constexpr float jump_height = 2.0f;
  constexpr float max_step = 1.0f / 60.0f;

  impl->yaw += input.look_delta.x * sensitivity;
  impl->pitch -= input.look_delta.y * sensitivity;
  constexpr float pitch_limit = JPH::DegreesToRadians(90.0f) - 0.01f;
  impl->pitch = std::clamp(impl->pitch, -pitch_limit, pitch_limit);

  const float cy = std::cos(impl->yaw);
  const float sy = std::sin(impl->yaw);
  Vector3 horizontal{
    cy * input.movement.x - sy * input.movement.y,
    0.0f,
    sy * input.movement.x + cy * input.movement.y
  };
  float speed_scale = std::sqrt(
    horizontal.x * horizontal.x + horizontal.z * horizontal.z
  );
  if (speed_scale > 0.0f) {
    horizontal.x *= move_speed / speed_scale;
    horizontal.z *= move_speed / speed_scale;
  }

  float remaining = std::clamp(frame_dt, 0.0f, 0.1f);
  bool jump = input.jump;
  CharacterLayerFilter filter{impl->layer};
  auto& character = *impl->character;

  while (remaining > 0.0f) {
    float dt = std::min(remaining, max_step);
    impl->world->update(dt);

    bool on_ground =
      character.GetGroundState() ==
      JPH::CharacterBase::EGroundState::OnGround;

    float vertical = character.GetLinearVelocity().GetY();
    if (on_ground && vertical <= 0.0f) {
      vertical = 0.0f;
    }
    if (jump && on_ground) {
      vertical = std::sqrt(-2.0f * gravity * jump_height);
    } else {
      vertical += gravity * dt;
    }
    jump = false;

    character.SetLinearVelocity(JPH::Vec3{
      horizontal.x, vertical, horizontal.z
    });
    character.Update(
      dt, JPH::Vec3{0.0f, gravity, 0.0f},
      JPH::BroadPhaseLayerFilter{}, filter,
      JPH::BodyFilter{}, JPH::ShapeFilter{},
      impl->world->impl->temp_allocator
    );

    remaining -= dt;
  }

  Vector3 feet = position();
  float cp = std::cos(impl->pitch);
  impl->camera.position = {
    feet.x, feet.y + 1.62f, feet.z
  };
  impl->camera.target = {
    feet.x + cp * std::cos(impl->yaw),
    feet.y + 1.62f + std::sin(impl->pitch),
    feet.z + cp * std::sin(impl->yaw)
  };
}

Vector3 PlayerController::position() const {
  return from_jolt_vec(impl->character->GetPosition());
}

Vector3 PlayerController::velocity() const {
  return from_jolt_vec(impl->character->GetLinearVelocity());
}

bool PlayerController::grounded() const {
  return impl->character->GetGroundState() ==
    JPH::CharacterBase::EGroundState::OnGround;
}

BoundingBox PlayerController::bounds() const {
  Vector3 feet = position();
  const float half_width = impl->size.x * 0.5f;
  const float half_depth = impl->size.z * 0.5f;
  return {
    .min = {
      feet.x - half_width,
      feet.y,
      feet.z - half_depth
    },
    .max = {
      feet.x + half_width,
      feet.y + impl->size.y,
      feet.z + half_depth
    }
  };
}

const Camera3D& PlayerController::camera() const {
  return impl->camera;
}
