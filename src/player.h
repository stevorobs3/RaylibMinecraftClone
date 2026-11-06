#pragma once
#include <raylib.h>
#include <raymath.h>

struct Player {
		// center of the feet
		Vector3 position{};
		Vector3 velocity{};
		bool grounded = false;

		BoundingBox get_bounds() {
			auto bb = BoundingBox();
			bb.min = position - Vector3(player_width / 2, 0, player_width / 2);
			bb.max = position + Vector3(player_width / 2, player_height, player_width / 2);
			return bb;
		}

	private:
		static constexpr float player_width = 0.6f;
		static constexpr float player_height = 1.8f;
};