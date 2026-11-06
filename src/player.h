#pragma once
#include <raylib.h>
#include <raymath.h>
#include <optional>
#include <format>
#include <iostream>

#include "vector.h"
#include "world.h"

struct BlockHit {
	Vector3Int position;
	Vector3Int face_normal;
};


struct Player {
	// positioning and movement
	Vector3 position{}; // center of the feet
	Vector3 velocity{};
	bool grounded = false;

	BoundingBox get_bounds() const;
	void apply_gravity(float dt);
	void apply_collisions(const World &world, const Vector3 previous_position);


	// targeting
	std::optional<BlockHit> target = std::nullopt;
	
	void find_target(const World &world, const Camera &camera);

	// block placement and removal
	// todo: might fit better in a tools class, later

	void manipulate_blocks(World &world, bool should_place_block, bool should_delete_block);

	friend std::ostream& operator<<(std::ostream &os, const Player &player) {
		
		std::string player_str = std::format("Player(pos= {},{},{}, vel={},{},{})", 
			player.position.x,player.position.y,player.position.z,
			player.velocity.x,player.velocity.y,player.velocity.z);

		os << player_str;
		return os;
	}

private:
	static constexpr float player_width = 0.6f;
	static constexpr float player_height = 1.8f;
	static constexpr float gravity = -9.81f;
};