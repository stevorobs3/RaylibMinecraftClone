#include "doctest.h"
#include "rendering/mesh_generator.h"
#include "rendering/chunk_snapshot.h"
#include <numeric>

TEST_CASE("Empty section produces no geometry") {
  World world;
  TextureAtlas atlas;
  MeshGenerator generator(atlas);
  auto* section = world.find_section({{0, 0}, 3});
  REQUIRE(section != nullptr);
  auto result = generator.build(make_snapshot(world, *section));
  CHECK(result.empty());
}

TEST_CASE(
  "Single stone and two adjacent stones emit correct exposed faces"
) {
  World world;
  TextureAtlas atlas;
  MeshGenerator generator(atlas);
  auto* section = world.find_section({{0, 0}, 3});
  REQUIRE(section != nullptr);
  world.set(8, 56, 8, BlockType::STONE);
  auto one = generator.build(make_snapshot(world, *section));
  REQUIRE(one.size() == 1);
  CHECK(one[0].vertex_count == 24);
  CHECK(one[0].triangle_count == 12);
  CHECK(one[0].indices.size() == 36);
  CHECK(one[0].valid());
  for (unsigned short index:one[0].indices)
    CHECK(index < one[0].vertex_count);
  world.set(9, 56, 8, BlockType::STONE);
  auto two = generator.build(make_snapshot(world, *section));
  REQUIRE(two.size() == 1);
  CHECK(two[0].vertex_count == 40);
  CHECK(two[0].triangle_count == 20);
}

TEST_CASE("Grass block includes a tinted overlay on four sides") {
  World world;
  TextureAtlas atlas;
  MeshGenerator generator(atlas);
  auto* section = world.find_section({{0, 0}, 3});
  world.set(8, 56, 8, BlockType::GRASS);
  auto mesh = generator.build(make_snapshot(world, *section));
  REQUIRE(mesh.size() == 1);
  CHECK(mesh[0].vertex_count == 40);
  CHECK(mesh[0].triangle_count == 20);
}

TEST_CASE("Solid section does not emit internal faces") {
  World world;
  TextureAtlas atlas;
  MeshGenerator generator(atlas);
  auto* section = world.find_section({{0, 0}, 3});
  REQUIRE(section != nullptr);
  world.fill_section(section->coord, BlockType::STONE);
  const auto mesh = generator.build(make_snapshot(world, *section));
  REQUIRE(mesh.size() == 1);
  CHECK(mesh[0].vertex_count == 6*16*16*4);
}

TEST_CASE(
  "Checkerboard with grass overlays splits across the 16-bit index limit"
) {
  World world;
  TextureAtlas atlas;
  MeshGenerator generator(atlas);
  for (int x = 0; x < 16; x++)
    for (int y = 48; y < 64; y++)
      for (int z = 0; z < 16; z++)
        if ((x+y+z)%2 == 0)
          world.set(x, y, z, BlockType::GRASS);
  const auto* section = world.find_section({{0, 0}, 3});
  const auto meshes   = generator.build(make_snapshot(world, *section));
  REQUIRE(meshes.size() == 2);
  int vertices = 0;
  for (const auto& mesh:meshes) {
    CHECK(mesh.vertex_count <= 65536);
    CHECK(mesh.indices.size() ==
      static_cast<size_t>(mesh.triangle_count*3));
    for (unsigned short index:mesh.indices)
      CHECK(index < mesh.vertex_count);
    vertices += mesh.vertex_count;
  }
  CHECK(vertices == 2048*10*4);
}

TEST_CASE("Malformed mesh buffers fail validation before GPU upload") {
  MeshData bad;
  CHECK_FALSE(bad.valid());
  World world;
  TextureAtlas atlas;
  MeshGenerator generator(atlas);
  world.set(8, 56, 8, BlockType::STONE);
  auto data = generator.build(make_snapshot(world,
    *world.find_section({{0, 0}, 3})));
  REQUIRE(data.size() == 1);
  CHECK(data[0].valid());
  data[0].vertices.pop_back();
  CHECK_FALSE(data[0].valid());
}
