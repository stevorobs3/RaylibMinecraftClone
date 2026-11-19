#include "doctest.h"
#include "rendering/chunk_snapshot.h"
#include "rendering/mesh_generator.h"

TEST_CASE("Cross-chunk neighbour faces disappear when the next chunk has a block") {
  World world;
  TextureAtlas atlas;
  MeshGenerator generator(atlas);
  const auto* left  = world.find_section({{-1, 0}, 3});
  const auto* right = world.find_section({{0, 0}, 3});
  REQUIRE(left != nullptr);
  REQUIRE(right != nullptr);
  world.set(-1, 56, 0, BlockType::STONE);
  auto before = generator.build(make_snapshot(world, *left));
  REQUIRE(before.size() == 1);
  CHECK(before[0].vertex_count == 24);
  world.set(0, 56, 0, BlockType::STONE);
  auto left_mesh  = generator.build(make_snapshot(world, *left));
  auto right_mesh = generator.build(make_snapshot(world, *right));
  REQUIRE(left_mesh.size() == 1);
  REQUIRE(right_mesh.size() == 1);
  CHECK(left_mesh[0].vertex_count == 20);
  CHECK(right_mesh[0].vertex_count == 20);
}

TEST_CASE("Cross-section neighbour faces disappear across Y=63/64") {
  World world;
  TextureAtlas atlas;
  MeshGenerator generator(atlas);
  world.set(8, 63, 8, BlockType::STONE);
  world.set(8, 64, 8, BlockType::STONE);
  auto* low  = world.find_section({{0, 0}, 3});
  auto* high = world.find_section({{0, 0}, 4});
  REQUIRE(low != nullptr);
  REQUIRE(high != nullptr);
  auto a = generator.build(make_snapshot(world, *low));
  auto b = generator.build(make_snapshot(world, *high));
  REQUIRE(a.size() == 1);
  REQUIRE(b.size() == 1);
  CHECK(a[0].vertex_count == 20);
  CHECK(b[0].vertex_count == 20);
}

TEST_CASE("Missing neighbour chunks expose outward facing geometry") {
  World world;
  TextureAtlas atlas;
  MeshGenerator generator(atlas);
  world.set(-1, 56, 0, BlockType::STONE);
  world.set(0, 56, 0, BlockType::STONE);
  CHECK(world.unload_chunk({0, 0}));
  const auto* left = world.find_section({{-1, 0}, 3});
  REQUIRE(left != nullptr);
  auto mesh = generator.build(make_snapshot(world, *left));
  REQUIRE(mesh.size() == 1);
  CHECK(mesh[0].vertex_count == 24);
}
