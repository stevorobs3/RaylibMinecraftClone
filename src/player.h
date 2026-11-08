#pragma once
#include <raylib.h>
#include <raymath.h>
#include <optional>
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

	//camera angle in radians
	float yaw = 0.0f;
	float pitch = 0.0f;


	BoundingBox get_bounds() const;
	void apply_physics(float dt);
	void apply_xz_movement(Vector3 direction, const Camera &camera, float dt);
	void apply_collisions(const World &world, const Vector3 previous_position);
	void jump();

	// targeting
	std::optional<BlockHit> target = std::nullopt;
	
	void find_target(const World &world, const Camera &camera);

	// block placement and removal
	// todo: might fit better in a tools class, later

	void manipulate_blocks(World &world, bool should_place_block, bool should_delete_block);

	// camera movement
	Vector3 get_forward() const;
	Camera3D get_camera() const;


private:

	static constexpr float player_width = 0.6f; // m
	static constexpr float player_height = 1.8f; // m
	static constexpr float gravity = -9.81f; // m/s/s
	static constexpr float jump_height = 2.0f; // m
	static constexpr float xz_movement_speed = 4.0f; // in m/s
};

std::ostream& operator<<(std::ostream &os, const Player &player);
