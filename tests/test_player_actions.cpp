#include "doctest.h"
#include "player/player.h"
#include "player_input.h"

TEST_CASE("Input maps to an action independent of the player") {
  PlayerInput input{};
  CHECK(input.block_action() == BlockAction::None);
  input.place_block = true;
  CHECK(input.block_action() == BlockAction::Place);
  input.remove_block = true;
  CHECK(input.block_action() == BlockAction::Remove);
}
TEST_CASE("Removing blocks and preserving bedrock") {
  World world;
  WorldPhysics physics(world);
  Player player;
  world.set(0, 7, 0, BlockType::STONE);
  player.target = BlockHit{{0, 7, 0}, {0, 1, 0}};
  player.manipulate_blocks(world, physics, {{3, 7, 3}, {4, 9, 4}},
    BlockAction::Remove);
  CHECK(world.get(0, 7, 0) == BlockType::AIR);
  CHECK_FALSE(player.target.has_value());
  player.target = BlockHit{{0, 0, 0}, {0, 1, 0}};
  player.manipulate_blocks(world, physics, {{3, 7, 3}, {4, 9, 4}},
    BlockAction::Remove);
  CHECK(world.get(0, 0, 0) == BlockType::BEDROCK);
}
TEST_CASE("Placing blocks rejects overlap with the player") {
  World world;
  WorldPhysics physics(world);
  Player player;
  world.set(0, 7, 0, BlockType::STONE);
  player.target = BlockHit{{0, 7, 0}, {0, 1, 0}};
  player.manipulate_blocks(world, physics, {{0, 8, 0}, {1, 9, 1}},
    BlockAction::Place);
  CHECK(world.get(0, 8, 0) == BlockType::AIR);
  player.manipulate_blocks(world, physics, {{3, 8, 3}, {4, 9, 4}},
    BlockAction::Place);
  CHECK(world.get(0, 8, 0) == BlockType::DIRT);
}
TEST_CASE("Ray targeting finds the closest intersected block") {
  World world;
  WorldPhysics physics(world);
  Player player;
  world.set(0, 8, 0, BlockType::STONE);
  world.set(0, 8, 1, BlockType::DIRT);
  const Ray ray{{0.5f, 8.5f, -2.5f}, {0, 0, 1}};
  player.find_target(world, ray);
  REQUIRE(player.target.has_value());
  CHECK(player.target->position == Vector3Int(0, 8, 0));
  CHECK(player.target->face_normal == Vector3Int(0, 0, -1));
}
