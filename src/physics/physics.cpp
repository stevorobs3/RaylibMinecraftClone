#include "physics.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

struct PhysicsWorld::Impl {
		JPH::PhysicsSystem system;
};

PhysicsWorld::PhysicsWorld()
		: impl(std::make_unique<Impl>()) {}

PhysicsWorld::~PhysicsWorld() = default;

void PhysicsWorld::update(float dt) {
		// Physics simulation comes next.
}