#include "doctest.h"
#include "world/world.h"

TEST_CASE("Chunk section fill returns whether its contents changed") {
  World world;
  auto& section = world.get_chunk_section(0, 3);
  CHECK_FALSE(section.fill(BlockType::AIR));
  CHECK(section.fill(BlockType::STONE));
  CHECK(section.const_blocks()[3,4,5] == BlockType::STONE);
  CHECK_FALSE(section.fill(BlockType::STONE));
  CHECK(section.fill(BlockType::AIR));
}

TEST_CASE("Chunk section local coordinate bounds") {
  World world;
  const auto& section = world.get_chunk_section(0,0);
  CHECK(section.contains({0,0,0}));
  CHECK(section.contains({15,15,15}));
  CHECK_FALSE(section.contains({16,0,0}));
  CHECK_FALSE(section.contains({-1,0,0}));
}
