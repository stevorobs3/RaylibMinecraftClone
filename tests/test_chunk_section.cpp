#include "doctest.h"
#include "world/world.h"
#include <type_traits>
#include <tuple>
static_assert(!std::is_copy_constructible_v<Chunk>);
static_assert(!std::is_move_constructible_v<Chunk>);
static_assert(!std::is_copy_constructible_v<ChunkSection>);
static_assert(!std::is_move_constructible_v<ChunkSection>);
static_assert(std::tuple_size_v<decltype(Chunk::chunk_sections)> == 24);

TEST_CASE(
  "Fixed 24 section array has consistent negative-coordinate origins"
) {
  World world;
  const Chunk* chunk = world.find_chunk({-1, 0});
  REQUIRE(chunk != nullptr);
  CHECK(chunk->chunk_sections.size() == 24);
  CHECK(chunk->chunk_sections[0].origin == Vector3Int(-16, 0, 0));
  CHECK(chunk->chunk_sections[23].origin == Vector3Int(-16, 368, 0));
  CHECK(chunk->bounds.max.y == 384.0f);
}

TEST_CASE("Local fill, revision, acknowledgement and bounds") {
  World world;
  auto* s = world.find_section({{0, 0}, 23});
  REQUIRE(s != nullptr);
  s->acknowledge(s->revision);
  CHECK_FALSE(s->dirty);
  CHECK(world.fill_section(s->coord, BlockType::AIR) ==
    SetBlockResult::Unchanged);
  const auto version = s->revision;
  CHECK(world.fill_section(s->coord, BlockType::STONE) ==
    SetBlockResult::Changed);
  CHECK(s->dirty);
  CHECK(s->revision>version);
  CHECK(s->get_local({3, 4, 5}) == BlockType::STONE);
  s->acknowledge(version);
  CHECK(s->dirty);
  s->acknowledge(s->revision);
  CHECK_FALSE(s->dirty);
  CHECK(world.fill_section(s->coord, BlockType::STONE) ==
    SetBlockResult::Unchanged);
  CHECK(s->contains({0, 0, 0}));
  CHECK(s->contains({15, 15, 15}));
  CHECK_FALSE(s->contains({16, 0, 0}));
  CHECK_FALSE(s->contains({-1, 0, 0}));
}
