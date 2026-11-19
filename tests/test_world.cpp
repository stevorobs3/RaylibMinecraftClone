#include "doctest.h"
#include "world/world.h"
#include <stdexcept>

TEST_CASE("World initialisation and block editing") {
  World world;
  CHECK(world.get(0, 0, 0) == BlockType::BEDROCK);
  CHECK(world.get(0, 6, 0) == BlockType::GRASS);
  CHECK(world.get(0, 7, 0) == BlockType::AIR);
  world.dirty_chunk_sections.clear();
  world.set(2, 7, 2, BlockType::DIRT);
  CHECK(world.get(2, 7, 2) == BlockType::DIRT);
  CHECK(world.dirty_chunk_sections.contains({0, 0}));
  world.dirty_chunk_sections.clear();
  world.set(2, 7, 2, BlockType::DIRT);
  CHECK(world.dirty_chunk_sections.empty());
  world.initialise();
  CHECK(world.get(2, 7, 2) == BlockType::AIR);
}

TEST_CASE("World coordinates and out-of-bounds access") {
  World world;
  CHECK(world.contains({0,0,0}));
  CHECK(world.contains({World::width - 1, World::height - 1, World::width - 1}));
  CHECK_FALSE(world.contains({-1,0,0}));
  CHECK_FALSE(world.contains({World::width,0,0}));
  CHECK_THROWS_AS(world.get(-1, 7, 0), std::out_of_range);
  CHECK_THROWS_AS(world.set(World::width, 7, 0, BlockType::DIRT), std::out_of_range);
  CHECK(world.to_grid_position({-0.1f, 0.1f, -16.1f}) == Vector3Int{-1,0,-17});
}

TEST_CASE("Dirty propagation across chunk and section boundaries") {
  World world;
  world.dirty_chunk_sections.clear();
  world.set(15, 15, 15, BlockType::STONE);
  CHECK(world.dirty_chunk_sections.contains({0,0}));
  CHECK(world.dirty_chunk_sections.contains({1,0}));
  CHECK(world.dirty_chunk_sections.contains({2,0}));
  CHECK(world.dirty_chunk_sections.contains({0,1}));
  CHECK(world.dirty_chunk_sections.size() == 4);
}

TEST_CASE("Block storage maps to the correct chunk and section") {
  World world;
  world.set(16, 16, 16, BlockType::STONE);
  CHECK(world.get(16, 16, 16) == BlockType::STONE);
  CHECK(world.get(0, 16, 0) == BlockType::AIR);
}
