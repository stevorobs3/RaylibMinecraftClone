#include "texture_atlas.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <external/glad.h>

std::array<Vector2, 4>
TextureAtlas::lookup_texture_uv(TextureId id) const noexcept {
  const int index  = static_cast<int>(id);
  const int column = index % grid_size;
  const int row    = index / grid_size;

  const int cell = tile_pixels + 2 * border_pixels;
  const float texel = 1.0f / static_cast<float>(
    mip_atlas_pixels(0)
  );

  // UVs cover only the original image, not its extruded border.
  // The border is sampled only when filtering near a tile edge.
  const float u    = (column * cell + border_pixels) * texel;
  const float v    = (row * cell + border_pixels) * texel;
  const float step = tile_pixels * texel;

  return {{
    {u, v},
    {u + step, v},
    {u, v + step},
    {u + step, v + step}
  }};
}

ImageMips TextureAtlas::load_image_with_mips(
  const fs::path& path
) {
  ImageMips mips{};
  const std::string file = path.string();

  mips[0].reset(LoadImage(file.c_str()));

  if (!mips[0].valid()) {
    TraceLog(LOG_ERROR, "Failed to load image: %s", file.c_str());
    return {};
  }

  ImageFormat(
    &mips[0].image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
  );

  if (!mips[0].valid())
    return {};

  for (int i = 1; i < MIP_LEVELS; i++) {
    const Image& prev = mips[i - 1].image;

    mips[i].reset(ImageCopy(prev));

    if (!mips[i].valid())
      return {};

    ImageResize(
      &mips[i].image,
      std::max(1, prev.width / 2),
      std::max(1, prev.height / 2)
    );

    if (!mips[i].valid())
      return {};
  }

  return mips;
}

bool TextureAtlas::same_size(
  const AllImageMips& all, int level
) noexcept {
  const Image& first = all[0][level].image;

  if (!IsImageValid(first))
    return false;

  for (const auto& mips : all) {
    const Image& image = mips[level].image;

    if (!IsImageValid(image) ||
        image.width != first.width ||
        image.height != first.height ||
        image.format != first.format) {
      return false;
    }
  }

  return true;
}

std::optional<ImageMips> TextureAtlas::stitch_images(
  const AllImageMips& all
) {
  ImageMips output{};

  for (int level = 0; level < MIP_LEVELS; level++) {
    if (!same_size(all, level))
      return std::nullopt;

    const int tile   = mip_tile_pixels(level);
    const int border = mip_border_pixels(level);
    const int cell   = tile + 2 * border;
    const int width  = mip_atlas_pixels(level);

    // Exact halving between levels keeps OpenGL mipmaps complete.
    const Image& first = all[0][level].image;

    if (first.width != tile || first.height != tile ||
        first.format != PIXELFORMAT_UNCOMPRESSED_R8G8B8A8) {
      TraceLog(
        LOG_ERROR, "Atlas expects %dx%d RGBA tiles at mip %d",
        tile, tile, level
      );
      return std::nullopt;
    }

    output[level].reset(GenImageColor(width, width, BLANK));

    if (!output[level].valid())
      return std::nullopt;

    // Each tile fills its cell, including the replicated border.
    // Copying RGBA directly preserves the original alpha mask.
    auto* dst = static_cast<unsigned char*>(
      output[level].image.data
    );

    for (int index = 0;
         index < static_cast<int>(file_names.size());
         index++) {
      const Image& source = all[index][level].image;
      const auto* src = static_cast<const unsigned char*>(
        source.data
      );
      const int left = (index % grid_size) * cell;
      const int top  = (index / grid_size) * cell;

      for (int y = 0; y < cell; y++) {
        const int sy = std::clamp(y - border, 0, tile - 1);

        for (int x = 0; x < cell; x++) {
          const int sx = std::clamp(x - border, 0, tile - 1);
          const int dst_index =
            ((top + y) * width + left + x) * 4;
          const int src_index = (sy * tile + sx) * 4;

          std::memcpy(
            dst + dst_index, src + src_index, 4
          );
        }
      }
    }
  }

  return output;
}

Texture2D TextureAtlas::to_texture(
  const ImageMips& mips
) noexcept {
  GLuint id = 0;
  glGenTextures(1, &id);

  if (id == 0)
    return {};

  glBindTexture(GL_TEXTURE_2D, id);

  for (int level = 0; level < MIP_LEVELS; level++) {
    const Image& image = mips[level].image;

    glTexImage2D(
      GL_TEXTURE_2D, level, GL_RGBA8,
      image.width, image.height, 0,
      GL_RGBA, GL_UNSIGNED_BYTE, image.data
    );
  }

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
  glTexParameteri(
    GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MIP_LEVELS - 1
  );
  glTexParameteri(
    GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR
  );
  glTexParameteri(
    GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST
  );

  glBindTexture(GL_TEXTURE_2D, 0);

  if (glGetError() != GL_NO_ERROR) {
    glDeleteTextures(1, &id);
    return {};
  }

  return Texture2D{
    id,
    mips[0].image.width,
    mips[0].image.height,
    MIP_LEVELS,
    PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
  };
}

bool TextureAtlas::load() {
  AllImageMips all{};
  const auto directory =
    fs::path(GetApplicationDirectory()) / "assets";

  for (int i = 0; i < static_cast<int>(file_names.size()); i++) {
    all[i] = load_image_with_mips(directory / file_names[i]);

    if (!all[i][0].valid())
      return false;
  }

  auto stitched = stitch_images(all);

  if (!stitched)
    return false;

  const Texture2D replacement = to_texture(*stitched);

  if (replacement.id == 0)
    return false;

  unload();
  texture_ = replacement;
  return true;
}

void TextureAtlas::unload() noexcept {
  if (texture_.id != 0)
    UnloadTexture(texture_);

  texture_ = {};
}
