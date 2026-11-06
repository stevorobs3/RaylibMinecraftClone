#pragma once
#include <raylib.h>
#include <raymath.h>

struct Player {
		// center of the feet
		Vector3 position{};
		Vector3 velocity{};
		bool grounded = false;

		BoundingBox get_bounds() const {
			auto bb = BoundingBox();
			bb.min = position - Vector3(player_width / 2, 0, player_width / 2);
			bb.max = position + Vector3(player_width / 2, player_height, player_width / 2);
			return bb;
		}

		void update_physics(float dt) {
			position.y += velocity.y * dt + 0.5 * dt * dt;
			velocity.y += dt * gravity;
		}

	private:
		static constexpr float player_width = 0.6f;
		static constexpr float player_height = 1.8f;
		static constexpr float gravity = -9.81f;
};