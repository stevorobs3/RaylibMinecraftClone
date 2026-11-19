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
  "Padded atlas UVs exclude gutters and neighbouring cells"
) {
  const TextureAtlas atlas;
  constexpr int tile = TextureAtlas::tile_pixels;
  constexpr int border = TextureAtlas::border_pixels;
  constexpr int cell = tile + 2 * border;
  constexpr int width = TextureAtlas::mip_atlas_pixels(0);

  auto dirt = atlas.lookup_texture_uv(TextureId::Dirt);
  CHECK(dirt[0].x ==
        doctest::Approx(static_cast<float>(border) / width));
  CHECK(dirt[0].y ==
        doctest::Approx(static_cast<float>(border) / width));
  CHECK(dirt[3].x ==
        doctest::Approx(
          static_cast<float>(border + tile) / width
        ));

  auto overlay = atlas.lookup_texture_uv(
    TextureId::GrassSideOverlay
  );
  CHECK(overlay[0].x ==
        doctest::Approx(
          static_cast<float>(2 * cell + border) / width
        ));
  CHECK(overlay[0].y ==
        doctest::Approx(
          static_cast<float>(cell + border) / width
        ));
  CHECK(overlay[3].x < 1.0f);
  CHECK(overlay[3].y < 1.0f);

  for (int level = 0; level < MIP_LEVELS; level++) {
    CHECK(TextureAtlas::mip_border_pixels(level) >= 1);
    if (level > 0) {
      CHECK(TextureAtlas::mip_atlas_pixels(level) * 2 ==
            TextureAtlas::mip_atlas_pixels(level - 1));
    }
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
