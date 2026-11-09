#include "player_controller.h"
#include "world_physics.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {

void check(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << '\n';
    std::exit(EXIT_FAILURE);
  }
}

BoundingBox player_at(Vector3 feet) {
  return {
    .min = {feet.x - 0.3f, feet.y, feet.z - 0.3f},
    .max = {feet.x + 0.3f, feet.y + 1.8f, feet.z + 0.3f}
  };
}

} // namespace

int main() {
  World world;
  WorldPhysics physics(world);

  check(physics.is_grounded(player_at({2.5f, 1.0f, 2.5f})),
        "Player supported by floor");
  check(!physics.is_grounded(player_at({2.5f, 1.5f, 2.5f})),
        "Midair player is not supported");
  check(physics.is_grounded(player_at({2.5f, 2.0f, 4.3f})),
        "Player standing partly on wall top");
  check(!physics.is_grounded(player_at({2.5f, 1.0f, 4.3f})),
        "Buried top between stacked blocks is not a floor");
  check(!physics.is_grounded(player_at({2.5f, 1.5f, 4.3f})),
        "Mid-wall seam is not a floor");
  check(physics.is_grounded(player_at({2.5f, 1.0f, -0.25f})),
        "Partial overhang stays supported");
  check(!physics.is_grounded(player_at({2.5f, 1.0f, -0.31f})),
        "No support past the end of a ledge");

  Vector3 feet{2.5f, 1.1f, 2.5f};
  Vector3 velocity{0.0f, -1.0f, 0.0f};
  physics.move_player(feet, velocity, {0.6f, 1.8f, 0.6f}, {0, -0.2f, 0});
  check(std::abs(feet.y - 1.0f) < 0.0001f && velocity.y == 0.0f,
        "Falling player stops exactly on floor");

  feet = {2.5f, 1.0f, 3.5f};
  velocity = {0.0f, 0.0f, 4.0f};
  physics.move_player(feet, velocity, {0.6f, 1.8f, 0.6f}, {0, 0, 0.8f});
  check(std::abs(feet.z - 3.7f) < 0.0001f && velocity.z == 0.0f,
        "Wall stops player at correct AABB boundary");

  Camera3D camera{
    .position = {2.5f, 2.62f, 2.5f},
    .target = {3.5f, 2.62f, 2.5f},
    .up = {0, 1, 0},
    .fovy = 75.0f,
    .projection = CAMERA_PERSPECTIVE
  };
  PlayerController player(camera, {2.5f, 1.0f, 2.5f}, {0.6f, 1.8f, 0.6f});
  player.reset({2.5f, 1.0f, 2.5f}, physics);
  player.update({.jump = true}, 1.0f / 60.0f, physics);
  check(player.velocity().y > 0.0f &&
        player.recent_jump_presses()[0].executed,
        "Jump fires while on actual floor");

  player.reset({2.5f, 1.5f, 4.3f}, physics);
  player.update({.jump = true}, 1.0f / 60.0f, physics);
  check(player.velocity().y < 0.0f &&
        !player.recent_jump_presses()[0].executed,
        "Jump does not fire against a wall seam");

  world.blocks[1, 2, 1].block_type = BlockType::Dirt;
  world.initialise();
  check(world.blocks[1, 2, 1].block_type == BlockType::Air,
        "Reset clears blocks placed during play");

  std::cout << "Voxel physics tests passed\n";
  return EXIT_SUCCESS;
}
