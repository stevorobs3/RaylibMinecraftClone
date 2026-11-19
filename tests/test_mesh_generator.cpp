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
  CHECK(one[0].texcoords2.size() == 48);
  CHECK(one[0].texcoords2[0] == -1.0f);
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
  CHECK(mesh[0].vertex_count == 24);
  CHECK(mesh[0].triangle_count == 12);
  CHECK(mesh[0].valid());

  // The same number of quads as an ordinary full cube.
  CHECK(mesh[0].indices.size() == 36);

  // Quad order is EAST, WEST, UP, DOWN, NORTH, SOUTH.
  // Four side faces use UV2; top and bottom only use UV1.
  for (int face = 0; face < 6; face++) {
    const bool has_overlay = face != 2 && face != 3;

    for (int vertex = 0; vertex < 4; vertex++) {
      const int i = (face * 4 + vertex) * 2;
      CHECK((mesh[0].texcoords2[i] >= 0.0f) == has_overlay);
      CHECK((mesh[0].texcoords2[i + 1] >= 0.0f) ==
            has_overlay);

      if (has_overlay) {
        const auto uv = atlas.lookup_texture_uv(
          TextureId::GrassSideOverlay
        );
        CHECK(mesh[0].texcoords2[i] ==
              doctest::Approx(uv[vertex].x));
        CHECK(mesh[0].texcoords2[i + 1] ==
              doctest::Approx(uv[vertex].y));
      }
    }
  }
}

TEST_CASE(
  "Grass overlay does not offset or duplicate physical cube faces"
) {
  World world;
  TextureAtlas atlas;
  MeshGenerator generator(atlas);

  world.set(8, 56, 8, BlockType::GRASS);
  const auto* section = world.find_section({{0, 0}, 3});
  REQUIRE(section != nullptr);

  const auto grass = generator.build(
    make_snapshot(world, *section)
  );
  REQUIRE(grass.size() == 1);

  world.set(8, 56, 8, BlockType::STONE);
  const auto stone = generator.build(
    make_snapshot(world, *section)
  );
  REQUIRE(stone.size() == 1);

  // Textures change, but the same physical faces are emitted.
  CHECK(grass[0].vertices == stone[0].vertices);
  CHECK(grass[0].indices == stone[0].indices);
  CHECK(grass[0].normals == stone[0].normals);
  CHECK(grass[0].vertex_count == stone[0].vertex_count);
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
  "Checkerboard with composited grass stays within 16-bit indices"
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
  REQUIRE(meshes.size() == 1);
  int vertices = 0;
  for (const auto& mesh:meshes) {
    CHECK(mesh.vertex_count <= 65536);
    CHECK(mesh.indices.size() ==
      static_cast<size_t>(mesh.triangle_count*3));
    for (unsigned short index:mesh.indices)
      CHECK(index < mesh.vertex_count);
    vertices += mesh.vertex_count;
  }
  CHECK(vertices == 2048 * 6 * 4);
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
  data[0].vertices.push_back(0.0f);
  CHECK(data[0].valid());
  data[0].texcoords2.pop_back();
  CHECK_FALSE(data[0].valid());
}
