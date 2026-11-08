#include "physics.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <algorithm>
#include <thread>

namespace {

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

} // namespace

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

    JPH::PhysicsSystem system;
};

PhysicsWorld::PhysicsWorld()
    : impl(nullptr)
{
    get_jolt_runtime();

    impl = std::make_unique<Impl>();
}

PhysicsWorld::~PhysicsWorld() = default;

void PhysicsWorld::update(float dt) {
    // Physics simulation comes next.
}