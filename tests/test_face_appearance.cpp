#include "doctest.h"
#include "rendering/face_appearance.h"
#include "rendering/texture_atlas.h"
#include "rendering/block_shader.h"

TEST_CASE("Texture layers are independent of block enum ordering") {
  auto grass_top = face_appearance(BlockType::GRASS, Direction::UP);
  CHECK(grass_top.base.texture == TextureId::GrassTop);
  CHECK_FALSE(grass_top.overlay.has_value());
  auto grass_bottom = face_appearance(BlockType::GRASS, Direction::DOWN);
  CHECK(grass_bottom.base.texture == TextureId::Dirt);
  CHECK_FALSE(grass_bottom.overlay.has_value());
  for (auto dir:{Direction::EAST, Direction::WEST, Direction::NORTH,
    Direction::SOUTH}) {
    const auto face = face_appearance(BlockType::GRASS, dir);
    CHECK(face.base.texture == TextureId::GrassSide);
    REQUIRE(face.overlay.has_value());
    CHECK(face.overlay->texture == TextureId::GrassSideOverlay);
  }
  CHECK(face_appearance(BlockType::STONE, Direction::UP).base.texture ==
    TextureId::Stone);
}

TEST_CASE(
  "Texture UV coordinates are normalised without an OpenGL context"
) {
  const TextureAtlas atlas;
  auto uv = atlas.lookup_texture_uv(TextureId::Dirt);
  CHECK(uv[0].x == doctest::Approx(0));
  CHECK(uv[0].y == doctest::Approx(0));
  CHECK(uv[3].x == doctest::Approx(1.0/3.0));
  CHECK(uv[3].y == doctest::Approx(1.0/3.0));
  auto overlay = atlas.lookup_texture_uv(TextureId::GrassSideOverlay);
  CHECK(overlay[0].x == doctest::Approx(2.0/3.0));
  CHECK(overlay[0].y == doctest::Approx(1.0/3.0));
  for (auto p:overlay) {
    CHECK(p.x >= 0); CHECK(p.x <= 1);
    CHECK(p.y >= 0); CHECK(p.y <= 1);
  }
}

TEST_CASE("Overlay alpha cutout never uses partial blending") {
  // Samples on opposite sides of the threshold select whole
  // layers, rather than blending base and overlay RGB values.
  CHECK_FALSE(BlockOverlay::visible(0.0f));
  CHECK_FALSE(BlockOverlay::visible(0.01f));
  CHECK_FALSE(BlockOverlay::visible(0.49f));
  CHECK(BlockOverlay::visible(0.5f));
  CHECK(BlockOverlay::visible(0.51f));
  CHECK(BlockOverlay::visible(1.0f));
}
