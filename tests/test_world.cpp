#include "doctest.h"
#include "world/world.h"
#include <type_traits>

TEST_CASE(
  "Initial terrain is centred on the origin and 24 sections tall"
) {
  World world;
  CHECK(world.chunks().size() == 4);
  CHECK(world.is_loaded({-1, -1}));
  CHECK(world.is_loaded({0, 0}));
  CHECK_FALSE(world.is_loaded({1, 0}));
  CHECK_FALSE(world.is_loaded({-2, 0}));
  CHECK(World::height == 384);
  CHECK(world.get(-16, 0, -16) == BlockType::BEDROCK);
  CHECK(world.get(-1, 6, -1) == BlockType::GRASS);
  CHECK(world.get(0, 6, 0) == BlockType::GRASS);
  CHECK(world.get(15, 7, 15) == BlockType::AIR);
  CHECK(world.contains({-16, 383, -16}));
  CHECK_FALSE(world.contains({-17, 7, -16}));
  CHECK_FALSE(world.contains({16, 7, 0}));
}

TEST_CASE(
  "World edits are explicit and can be checked without exceptions"
) {
  World world;
  CHECK(world.set(-1, 7, -1, BlockType::DIRT) == SetBlockResult::Changed);
  CHECK(world.get(-1, 7, -1) == BlockType::DIRT);
  CHECK(world.set(-1, 7, -1, BlockType::DIRT) ==
    SetBlockResult::Unchanged);
  CHECK(world.set(0, -1, 0, BlockType::DIRT) ==
    SetBlockResult::OutOfBounds);
  CHECK(world.set(0, 384, 0, BlockType::DIRT) ==
    SetBlockResult::OutOfBounds);
  CHECK(world.set(16, 7, 0, BlockType::DIRT) == SetBlockResult::Unloaded);
  CHECK_FALSE(world.try_get({16, 7, 0}).has_value());
  CHECK(world.try_get({0, 8, 0}).value() == BlockType::AIR);
  CHECK_FALSE(world.try_get({0, -1, 0}).has_value());
}

TEST_CASE(
  "Boundary edits invalidate both adjacent chunks and vertical sections"
) {
  World world;
  for (const auto& [coord, chunk] : world.chunks())
    for (const auto& s : chunk.chunk_sections) {
      auto* mutable_section = world.find_section(s.coord);
      mutable_section->acknowledge(mutable_section->revision);
    }

  const SectionCoord self{{-1, -1}, 0};
  const SectionCoord east{{0, -1}, 0};
  const SectionCoord south{{-1, 0}, 0};
  const SectionCoord upper{{-1, -1}, 1};
  const auto before = world.find_section(self)->revision;
  CHECK(world.set(-1, 15, -1, BlockType::STONE) ==
    SetBlockResult::Changed);
  CHECK(world.find_section(self)->dirty);
  CHECK(world.find_section(self)->revision > before);
  CHECK(world.find_section(east)->dirty);
  CHECK(world.find_section(south)->dirty);
  CHECK(world.find_section(upper)->dirty);
  const auto unchanged = world.find_section(self)->revision;
  CHECK(world.set(-1, 15, -1, BlockType::STONE) ==
    SetBlockResult::Unchanged);
  CHECK(world.find_section(self)->revision == unchanged);
}

TEST_CASE(
  "Chunk and section lookup preserve data across vertical and "
  "horizontal edges"
) {
  World world;
  CHECK(world.set(-16, 16, 0, BlockType::STONE) ==
    SetBlockResult::Changed);
  CHECK(world.get(-16, 16, 0) == BlockType::STONE);
  CHECK(world.get(0, 16, 0) == BlockType::AIR);
  CHECK(world.set(15, 383, 15, BlockType::DIRT) ==
    SetBlockResult::Changed);
  CHECK(world.get(15, 383, 15) == BlockType::DIRT);
}

TEST_CASE("Chunk map retains addresses when rehashing") {
  World world;
  Chunk* original = world.find_chunk({-1, -1});
  REQUIRE(original != nullptr);
  for (int x = 10; x < 40; x++)
    world.get_or_create_chunk({x, 3});
  CHECK(world.find_chunk({-1, -1}) == original);
}

TEST_CASE(
  "Explicit creation loads new coordinates without implicit reads"
) {
  World world;
  CHECK_FALSE(world.try_get({-33, 7, 0}).has_value());
  auto& chunk = world.get_or_create_chunk({-3, 0});
  CHECK(chunk.origin.x == -48);
  CHECK(world.try_get({-33, 7, 0}).value() == BlockType::AIR);
  CHECK(world.is_loaded({-3, 0}));
}

TEST_CASE(
  "Reset restores terrain and gives recreated chunks new generations"
) {
  World world;
  const auto old_generation = world.find_chunk({0, 0})->generation;
  world.set(0, 7, 0, BlockType::DIRT);
  world.initialise();
  CHECK(world.get(0, 7, 0) == BlockType::AIR);
  CHECK(world.find_chunk({0, 0})->generation != old_generation);
}

TEST_CASE(
  "Unloading a chunk invalidates its neighbours and removes "
  "block visibility"
) {
  World world;
  for (auto& [coord, chunk] : world.chunks())
    for (auto& section : chunk.chunk_sections)
      section.acknowledge(section.revision);
  REQUIRE(world.unload_chunk({0, 0}));
  CHECK_FALSE(world.is_loaded({0, 0}));
  CHECK_FALSE(world.try_get({0, 7, 0}).has_value());
  CHECK(world.find_section({{-1, 0}, 0})->dirty);
  CHECK(world.find_section({{0, -1}, 0})->dirty);
  CHECK_FALSE(world.unload_chunk({0, 0}));
}
