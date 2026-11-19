#pragma once

#include <array>
#include <filesystem>
#include <optional>

#include <raylib.h>

#include "face_appearance.h"

namespace fs = std::filesystem;

constexpr int MIP_LEVELS = 5;

struct OwnedImage {
  Image image{};

  OwnedImage() noexcept = default;
  explicit OwnedImage(Image raw) noexcept : image(raw) {}

  ~OwnedImage() noexcept {
    reset();
  }

  OwnedImage(const OwnedImage&) = delete;
  OwnedImage& operator=(const OwnedImage&) = delete;

  OwnedImage(OwnedImage&& other) noexcept
    : image(other.release()) {}

  OwnedImage& operator=(OwnedImage&& other) noexcept {
    if (this != &other)
      reset(other.release());

    return *this;
  }

  Image release() noexcept {
    Image result = image;
    image = {};
    return result;
  }

  void reset(Image next = {}) noexcept {
    if (image.data)
      UnloadImage(image);

    image = next;
  }

  bool valid() const noexcept {
    return IsImageValid(image);
  }
};

using ImageMips = std::array<OwnedImage, MIP_LEVELS>;

struct TextureAtlas {
  static constexpr std::array file_names{
    "dirt.png",
    "grass_block_top.png",
    "grass_block_side.png",
    "stone.png",
    "bedrock.png",
    "grass_block_side_overlay.png"
  };

  static constexpr int grid_size = 3;

  using AllImageMips = std::array<ImageMips, file_names.size()>;

  TextureAtlas() noexcept = default;
  ~TextureAtlas() noexcept { unload(); }

  TextureAtlas(const TextureAtlas&) = delete;
  TextureAtlas& operator=(const TextureAtlas&) = delete;
  TextureAtlas(TextureAtlas&&) = delete;
  TextureAtlas& operator=(TextureAtlas&&) = delete;

  std::array<Vector2, 4> lookup_texture_uv(
    TextureId id
  ) const noexcept;

  bool load();
  void unload() noexcept;

  Texture2D texture() const noexcept { return texture_; }

private:
  ImageMips load_image_with_mips(const fs::path& path);

  std::optional<ImageMips> stitch_images(
    const AllImageMips& all
  );

  static bool same_size(
    const AllImageMips& all, int level
  ) noexcept;

  static Texture2D to_texture(const ImageMips& mips) noexcept;

  Texture2D texture_{};
};
