#include "raylib.h"
#include "raymath.h"
#include <iostream>
#include <cmath>

#include "vector.h"
#include "world.h"
#include "player.h"


BoundingBox Player::get_bounds() const {
	auto bb = BoundingBox();
	bb.min = position - Vector3(player_width / 2, 0, player_width / 2);
	bb.max = position + Vector3(player_width / 2, player_height, player_width / 2);
	return bb;
}

// update y pos and y velocity based on world physics (gravity)
void Player::apply_physics(float dt) {
	position.y += velocity.y * dt + 0.5 * gravity * dt * dt;
	velocity.y += dt * gravity;
}

void Player::apply_xz_movement(Vector3 direction, const Camera &camera, float dt) {
		// Direction the camera is looking
	Vector3 forward = camera.target - camera.position;

	// Project onto the horizontal XZ plane
	forward.y = 0.0f;
	forward = Vector3Normalize(forward);

	// Calculate right direction
	Vector3 right = Vector3Normalize(
			Vector3CrossProduct(forward, camera.up)
	);

	// Convert local movement into world movement
	Vector3 movement = (right * direction.z) + (forward * direction.x);

	// Prevent faster diagonal movement
	if (Vector3LengthSqr(movement) > 0.0f) {
			movement = Vector3Normalize(movement);
	}

	position += movement * dt * xz_movement_speed;
	
}


bool boxes_overlap(const BoundingBox& a,
									 const BoundingBox& b) {
		return a.min.x < b.max.x &&
					 a.max.x > b.min.x &&
					 a.min.y < b.max.y &&
					 a.max.y > b.min.y &&
					 a.min.z < b.max.z &&
					 a.max.z > b.min.z;
}

// check whether player movement has caused collisions and update
// the player position accordingly - initially, just assuming y gravity movement
void Player::apply_collisions(const World &world, const Vector3 previous_position){
	// todo: this only works for gravity
	grounded = false;
	// todo: should still work while jumping
	if (velocity.y >= 0) return;

	// todo: pull into separate function? handle_gravity_collisions?
	std::optional<Vector3Int> crossed_block = std::nullopt;
	const BoundingBox player_bounds = get_bounds();
	const Rectangle player_xz = {
		player_bounds.min.x, player_bounds.min.z,
		player_bounds.max.x - player_bounds.min.x,
		player_bounds.max.z - player_bounds.min.z
	};
	for (auto&& [block, x, y, z] : world.blocks_with_positions()) {
		if (block.block_type == BlockType::Air) continue;
		// does player xz overlap block xz
		Rectangle block_xz  = Rectangle(x, z, 1, 1);
		float block_top = y + 1;
		bool crossed_block_top = previous_position.y >= block_top && position.y <= block_top;

		if (crossed_block_top && CheckCollisionRecs(player_xz, block_xz)) {
			// crossed this block - there could be multiple, stop at the one
			// with the biggest y, which there still could be multiple
			if (!crossed_block) {
				crossed_block = Vector3Int(x,y,z);
			}
			else if (crossed_block->y < y) {
				crossed_block = Vector3Int(x,y,z);
			}
			
		}
	}
	if (crossed_block) {
		velocity.y = 0;
		grounded = true;
		position.y = crossed_block->y + 1;
	}


	for (auto&& [block, x, y, z] : world.blocks_with_positions()) {
		if (block.block_type == BlockType::Air) continue;
		// todo: encapsulate box size somewhere
		auto box_bounds = BoundingBox();
		box_bounds.min = Vector3(x,y,z);
		box_bounds.max = Vector3(x + 1,y + 1,z + 1);
		if (boxes_overlap(get_bounds(), box_bounds)) {
			//todo: simplify and optimise
			auto old_position = position;
			position.x = previous_position.x;
			position.z = previous_position.z;
			// move in x, check boxes, reset if neccessary
			position.x = old_position.x;
			if (boxes_overlap(get_bounds(), box_bounds)) {
				position.x = previous_position.x;
			}
			position.z = old_position.z;
			if (boxes_overlap(get_bounds(), box_bounds)) {
				position.z = previous_position.z;
			}
			
			// move in z, check boxes, reset if neccessary
		}
	}


}

void Player::jump() {
	if (!grounded) return;

	float delta_vy = std::sqrt(std::abs(2 * gravity * jump_height));
	velocity.y += delta_vy;
	grounded = false;
}


void Player::find_target(const World &world, const Camera &camera) {
	auto screen_center = Vector2(GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f);
	auto ray = GetScreenToWorldRay(screen_center, camera);

	float max_reach = 5.0f;
	float distance = max_reach;

	// reset target
	target = std::nullopt;

	// todo: slow - check all world blocks
	for (auto &&[block, x, y, z] : world.blocks_with_positions()) {
		if (block.block_type == BlockType::Air) continue;
		
		auto bb = BoundingBox();
		bb.min = Vector3(x,y,z);
		bb.max = Vector3(x + 1,y + 1,z + 1);
		RayCollision rayCollision = GetRayCollisionBox(ray, bb);
		if (rayCollision.hit && rayCollision.distance < distance) {
			distance = rayCollision.distance;
			auto &normal = rayCollision.normal;
			target = BlockHit{
				.position = Vector3Int(x, y, z),
				.face_normal = Vector3Int(normal.x, normal.y, normal.z)
			};
		}		
	}
}


void Player::manipulate_blocks(World &world, bool should_place_block, bool should_delete_block) {
	if (!target) return;
	auto &t = *target;

	// prioritise placement
	if (should_delete_block) {
		auto &p = t.position;

		world.blocks[p.x, p.y, p.z].block_type = BlockType::Air;
		target.reset();
	}
	else if (should_place_block) {
		Vector3Int position = t.position + t.face_normal;

		if (world.contains(position)) {
			auto & b = world.blocks[position.x, position.y,position.z];
			if (b.block_type == BlockType::Air) {
				world.blocks[position.x, position.y,position.z].block_type = BlockType::Dirt;
				target.reset();
			}
		}
	}
}
