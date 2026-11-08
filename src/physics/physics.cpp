
#include "physics.h"
#include "world.h"

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
#include <unordered_map>
#include <vector>
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

class CharacterLayerFilter final : public JPH::ObjectLayerFilter {
public:
  explicit CharacterLayerFilter(PhysicsWorld::Layer layer) : layer_(layer) {}

  bool ShouldCollide(JPH::ObjectLayer other) const override {
    return can_collide(layer_, other);
  }

private:
  PhysicsWorld::Layer layer_;
};

class TerrainBodyFilter final : public JPH::BodyFilter {
public:
  explicit TerrainBodyFilter(
    const std::unordered_map<JPH::uint32, Vector3Int>& positions
  ) : positions_(positions) {}

  bool ShouldCollide(const JPH::BodyID& id) const override {
    return positions_.contains(id.GetIndexAndSequenceNumber());
  }

private:
  const std::unordered_map<JPH::uint32, Vector3Int>& positions_;
};

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

  std::vector<JPH::BodyID> owned_bodies;
  std::vector<JPH::BodyID> terrain_bodies;
  std::unordered_map<JPH::uint32, Vector3Int> terrain_positions;

  JPH::Ref<JPH::CharacterVirtual> player;
  PhysicsWorld::Layer player_layer = 0;

  void clear_terrain() {
    auto& bodies = system.GetBodyInterface();
    for (JPH::BodyID id : terrain_bodies) {
      bodies.RemoveBody(id);
      bodies.DestroyBody(id);
    }
    terrain_bodies.clear();
    terrain_positions.clear();
  }

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
    player = nullptr;
    clear_terrain();
    auto& bodies = system.GetBodyInterface();
    for (JPH::BodyID id : owned_bodies) {
      bodies.RemoveBody(id);
      bodies.DestroyBody(id);
    }
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
  impl->owned_bodies.push_back(id);

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

void PhysicsWorld::set_blocks(const World& world, Layer layer) {
  if (layer >= layer_count) {
    throw std::invalid_argument("Invalid terrain layer");
  }

  impl->clear_terrain();
  JPH::BoxShapeSettings shape_settings{JPH::Vec3{0.5f, 0.5f, 0.5f}};
  auto result = shape_settings.Create();
  if (result.HasError()) {
    throw std::runtime_error(result.GetError().c_str());
  }

  auto& bodies = impl->system.GetBodyInterface();
  for (auto&& [block, x, y, z] : world.blocks_with_positions()) {
    if (block.block_type == BlockType::Air) {
      continue;
    }

    JPH::BodyCreationSettings settings{
      result.Get(),
      JPH::RVec3{x + 0.5f, y + 0.5f, z + 0.5f},
      JPH::Quat::sIdentity(),
      JPH::EMotionType::Static,
      layer
    };

    JPH::BodyID id = bodies.CreateAndAddBody(
      settings, JPH::EActivation::DontActivate
    );
    if (id.IsInvalid()) {
      throw std::runtime_error("Failed to create terrain body");
    }
    impl->terrain_bodies.push_back(id);
    impl->terrain_positions.emplace(
      id.GetIndexAndSequenceNumber(), Vector3Int{x, y, z}
    );
  }

  if (impl->player != nullptr) {
    CharacterLayerFilter filter{impl->player_layer};
    impl->player->RefreshContacts(
      JPH::BroadPhaseLayerFilter{}, filter,
      JPH::BodyFilter{}, JPH::ShapeFilter{}, impl->temp_allocator
    );
  }
}

void PhysicsWorld::create_player(Vector3 feet, Vector3 size, Layer layer) {
  if (layer >= layer_count ||
      size.x <= 0.0f || size.y <= 0.0f || size.z <= 0.0f) {
    throw std::invalid_argument("Invalid player size or layer");
  }

  JPH::BoxShapeSettings box_settings{
    JPH::Vec3{size.x * 0.5f, size.y * 0.5f, size.z * 0.5f}
  };
  auto shape = box_settings.Create();
  if (shape.HasError()) {
    throw std::runtime_error(shape.GetError().c_str());
  }

  JPH::CharacterVirtualSettings settings;
  settings.mShape = shape.Get();
  settings.mShapeOffset = JPH::Vec3{0.0f, size.y * 0.5f, 0.0f};
  settings.mMaxSlopeAngle = JPH::DegreesToRadians(50.0f);

  impl->player_layer = layer;
  impl->player = new JPH::CharacterVirtual(
    &settings, JPH::RVec3{feet.x, feet.y, feet.z},
    JPH::Quat::sIdentity(), &impl->system
  );
  reset_player(feet);
}

void PhysicsWorld::reset_player(Vector3 feet) {
  if (impl->player == nullptr) {
    throw std::logic_error("Player has not been created");
  }

  impl->player->SetPosition(JPH::RVec3{feet.x, feet.y, feet.z});
  impl->player->SetLinearVelocity(JPH::Vec3::sZero());
  CharacterLayerFilter filter{impl->player_layer};
  impl->player->RefreshContacts(
    JPH::BroadPhaseLayerFilter{}, filter,
    JPH::BodyFilter{}, JPH::ShapeFilter{}, impl->temp_allocator
  );
}

void PhysicsWorld::move_player(
  Vector3 horizontal_velocity, bool jump, float dt
) {
  if (impl->player == nullptr || dt <= 0.0f) {
    return;
  }

  constexpr float gravity = -9.81f;
  constexpr float jump_height = 2.0f;
  const bool grounded =
    impl->player->GetGroundState() ==
    JPH::CharacterBase::EGroundState::OnGround;

  const auto current = impl->player->GetLinearVelocity();
  float vertical = current.GetY();
  if (grounded && vertical <= 0.0f) {
    vertical = 0.0f;
  }
  if (jump && grounded) {
    vertical = std::sqrt(-2.0f * gravity * jump_height);
  } else {
    vertical += gravity * dt;
  }

  impl->player->SetLinearVelocity(JPH::Vec3{
    horizontal_velocity.x, vertical, horizontal_velocity.z
  });

  CharacterLayerFilter filter{impl->player_layer};
  impl->player->Update(
    dt, JPH::Vec3{0, gravity, 0},
    JPH::BroadPhaseLayerFilter{}, filter,
    JPH::BodyFilter{}, JPH::ShapeFilter{}, impl->temp_allocator
  );
}

Vector3 PhysicsWorld::player_position() const {
  return impl->player
    ? from_jolt_vec(impl->player->GetPosition())
    : Vector3{};
}

Vector3 PhysicsWorld::player_velocity() const {
  return impl->player
    ? from_jolt_vec(impl->player->GetLinearVelocity())
    : Vector3{};
}

bool PhysicsWorld::player_grounded() const {
  return impl->player &&
    impl->player->GetGroundState() ==
    JPH::CharacterBase::EGroundState::OnGround;
}

std::optional<BlockHit> PhysicsWorld::raycast_block(
  Vector3 origin, Vector3 direction, float reach
) const {
  if (reach <= 0.0f) {
    return std::nullopt;
  }

  JPH::RRayCast ray{
    JPH::RVec3{origin.x, origin.y, origin.z},
    to_jolt_vec(direction) * reach
  };
  JPH::RayCastResult hit;
  TerrainBodyFilter filter{impl->terrain_positions};

  if (!impl->system.GetNarrowPhaseQuery().CastRay(
    ray, hit, JPH::BroadPhaseLayerFilter{},
    JPH::ObjectLayerFilter{}, filter
  )) {
    return std::nullopt;
  }

  auto it = impl->terrain_positions.find(
    hit.mBodyID.GetIndexAndSequenceNumber()
  );
  if (it == impl->terrain_positions.end()) {
    return std::nullopt;
  }

  JPH::BodyLockRead lock{impl->system.GetBodyLockInterface(), hit.mBodyID};
  if (!lock.Succeeded()) {
    return std::nullopt;
  }

  JPH::Vec3 normal = lock.GetBody().GetWorldSpaceSurfaceNormal(
    hit.mSubShapeID2, ray.GetPointOnRay(hit.mFraction)
  );

  return BlockHit{
    .position = it->second,
    .face_normal = Vector3Int{
      static_cast<int>(std::lround(normal.GetX())),
      static_cast<int>(std::lround(normal.GetY())),
      static_cast<int>(std::lround(normal.GetZ()))
    }
  };
}

void PhysicsWorld::update(float dt) {
  if (dt > 0.0f) {
    impl->system.Update(
      dt, 1, &impl->temp_allocator, &impl->job_system
    );
  }
}
