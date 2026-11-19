#include "doctest.h"
#include "rendering/mesh_generator.h"

TEST_CASE("Air-only section yields no geometry") {
  World world;
  world.get_chunk_section(0, 3).fill(BlockType::AIR);
  TextureAtlas atlas;
  // UV lookup only needs the atlas dimensions, not an OpenGL texture.
  atlas.texture.width = 192;
  atlas.texture.height = 192;
  MeshGenerator generator(atlas);
  CHECK_FALSE(generator.build(world.get_chunk_section(0, 3)).has_value());
}

TEST_CASE("Isolated full block emits six faces and valid indices") {
  World world;
  auto& section = world.get_chunk_section(0, 3);
  section.fill(BlockType::AIR);
  section.blocks()[8,8,8] = BlockType::STONE;
  TextureAtlas atlas;
  atlas.texture.width = 192;
  atlas.texture.height = 192;
  MeshGenerator generator(atlas);
  auto mesh = generator.build(section);
  REQUIRE(mesh.has_value());
  CHECK(mesh->vertex_count == 24);
  CHECK(mesh->triangle_count == 12);
  CHECK(mesh->indices.size() == 36);
  for (unsigned short index : mesh->indices) CHECK(index < mesh->vertex_count);
}

TEST_CASE("Adjacent full blocks hide their shared face") {
  World world;
  auto& section = world.get_chunk_section(0, 3);
  section.fill(BlockType::AIR);
  section.blocks()[8,8,8] = BlockType::STONE;
  section.blocks()[9,8,8] = BlockType::STONE;
  TextureAtlas atlas;
  atlas.texture.width = 192;
  atlas.texture.height = 192;
  MeshGenerator generator(atlas);
  auto mesh = generator.build(section);
  REQUIRE(mesh.has_value());
  CHECK(mesh->vertex_count == 40);
  CHECK(mesh->triangle_count == 20);
}
