#pragma once

#include <memory>

struct PhysicsWorld {
		PhysicsWorld();
		~PhysicsWorld();

		PhysicsWorld(const PhysicsWorld&) = delete;
		PhysicsWorld& operator=(
				const PhysicsWorld&) = delete;

		void update(float dt);

private:
		struct Impl;
		std::unique_ptr<Impl> impl;
};