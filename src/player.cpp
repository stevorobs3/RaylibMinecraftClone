#include "raylib.h"

#include "world.h"
#include "player.h"


BoundingBox Player::get_bounds() const {
	auto bb = BoundingBox();
	bb.min = position - Vector3(player_width / 2, 0, player_width / 2);
	bb.max = position + Vector3(player_width / 2, player_height, player_width / 2);
	return bb;
}

// update y pos and y velocity based on gravity
void Player::apply_gravity(float dt) {
	position.y += velocity.y * dt + 0.5 * gravity * dt * dt;
	velocity.y += dt * gravity;
}


// check whether player movement has caused collisions and update
// the player position accordingly - initially, just assuming y gravity movement
void Player::apply_collisions(const World &world, const Vector3 previous_position){
	for (auto&& [block, x, y, z] : world.blocks_with_positions()) {
		// todo: check block for collision
	}
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
