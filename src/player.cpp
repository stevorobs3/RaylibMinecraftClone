#include "raylib.h"
#include "raymath.h"

#include <cmath>
#include <format>
#include <iostream>

#include "player.h"
#include "vector.h"
#include "world.h"

BoundingBox Player::get_bounds() const {
  BoundingBox bb{};
  bb.min = position - Vector3(player_width / 2, 0, player_width / 2);
  bb.max = position + Vector3(player_width / 2, player_height, player_width / 2);
  return bb;
}

Vector3 Player::movement_velocity(Vector3 direction, const Camera& camera) const {
  Vector3 forward = camera.target - camera.position;
  forward.y = 0.0f;
  forward = Vector3Normalize(forward);
  Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
  Vector3 movement = right * direction.z + forward * direction.x;
  if (Vector3LengthSqr(movement) > 0.0f) {
    movement = Vector3Normalize(movement);
  }
  return movement * xz_movement_speed;
}

void Player::find_target(const PhysicsWorld& physics, const Camera& camera) {
  Vector2 centre{GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
  Ray ray = GetScreenToWorldRay(centre, camera);
  target = physics.raycast_block(ray.position, ray.direction, 5.0f);
}

bool Player::manipulate_blocks(World& world, bool place, bool remove) {
  if (!target) {
    return false;
  }

  BlockHit hit = *target;
  if (remove) {
    world.blocks[hit.position.x, hit.position.y, hit.position.z].block_type =
      BlockType::Air;
    target.reset();
    return true;
  }

  if (place) {
    Vector3Int position = hit.position + hit.face_normal;
    if (world.contains(position)) {
      auto& block = world.blocks[position.x, position.y, position.z];
      if (block.block_type == BlockType::Air) {
        block.block_type = BlockType::Dirt;
        target.reset();
        return true;
      }
    }
  }
  return false;
}

Vector3 Player::get_forward() const {
  return {
    std::cos(pitch) * std::cos(yaw),
    std::sin(pitch),
    std::cos(pitch) * std::sin(yaw)
  };
}

Camera3D Player::get_camera() const {
  Vector3 eye = position + Vector3{0, 1.62f, 0};
  return Camera3D{
    .position = eye,
    .target = eye + get_forward(),
    .up = {0, 1, 0},
    .fovy = 75.0f,
    .projection = CAMERA_PERSPECTIVE
  };
}

std::ostream& operator<<(std::ostream& os, const Player& player) {
  auto bb = player.get_bounds();
  os << std::format(
    "Player(pos={:.2f},{:.2f},{:.2f}, vel={:.2f},{:.2f},{:.2f}, bb={:.2f},{:.2f},{:.2f}->{:.2f},{:.2f},{:.2f})",
    player.position.x, player.position.y, player.position.z,
    player.velocity.x, player.velocity.y, player.velocity.z,
    bb.min.x, bb.min.y, bb.min.z,
    bb.max.x, bb.max.y, bb.max.z
  );
  return os;
}
