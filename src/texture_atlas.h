#pragma once
#include <filesystem>
#include <string>
#include <array>
#include <cassert>
#include <cmath>

#include "raylib.h"
#include "rlgl.h"
#include "external/glad.h"

#include "world.h"
#include "face.h"

namespace fs = std::filesystem;

// todo: pull implementation into .cpp file
struct TextureAtlas {
  Texture2D texture{};

  static constexpr int MIP_LEVELS = 5; // 64*64 ...4*4

  static constexpr std::array file_names{
    "dirt.png",
    "grass_block_top.png",
    "grass_block_side.png",
    "stone.png",
    "bedrock.png"
  };

  // convert into an n * n grid
  int n = std::ceil(std::sqrt(static_cast<float>(file_names.size())));


  using ImageMips = std::array<Image, MIP_LEVELS>;
  using AllImageMips = std::array<ImageMips, file_names.size()>;

  static constexpr int textures_lookup[][6]{
    // EAST WEST UP DOWN NORTH SOUTH
    {-1,-1,-1,-1,-1,-1}, // air - should never be used
    {2,2,1,0,2,2}, // grass
    {0,0,0,0,0,0}, // dirt
    {3,3,3,3,3,3}, // stone
    {4,4,4,4,4,4}, // bedrock
  };

  std::array<Vector2, 4> lookup_texture_uv(
    BlockType block_type,
    Face::Direction dir
  ) {
    int index = textures_lookup[static_cast<int>(block_type)]
      [static_cast<int>(dir)];
    int pos_x = index % n;
    int pos_y = index / n;
    //todo: this can be cached
    float texture_width  = static_cast<float>(texture.width) / n;
    float texture_height = static_cast<float>(texture.height) / n;
    Vector2 offset = Vector2(
      pos_x * texture_width,
      pos_y * texture_height
    );
    Vector2 texture_size = Vector2(texture.width, texture.height);
    /* we want all faces to be regular like this:
      0  1
      2  3
      triangle indexes are then:
      021 and  312
     */
    // UV coords are 0,0 in top left corner
    std::array<Vector2, 4> all_tex_coords{
      (offset + Vector2{0,             0})              / texture_size,
      (offset + Vector2{texture_width, 0})              / texture_size,
      (offset + Vector2{0,             texture_height}) / texture_size,
      (offset + Vector2{texture_width, texture_height}) / texture_size,
    };
    return all_tex_coords;
  }

  bool load() {

    const fs::path app_dir = fs::path(GetApplicationDirectory());

    const fs::path path_prefix = app_dir / "assets";

    AllImageMips all_image_mips{};
    for (int i = 0; i < file_names.size(); i++) {
      fs::path path = path_prefix / file_names[i];
      ImageMips mips = load_image_with_mips(path);
      if (mips.empty()) {
        return false;
      }
      all_image_mips[i] = mips;
    }

    ImageMips final_images = stitch_images(all_image_mips);
    for (Image image : final_images) {
      fs::path output_dir = app_dir / std::format("output-{}.png", image.width);
      bool saved = ExportImage(image, output_dir.string().c_str());
      if (!saved) {
        TraceLog(LOG_ERROR, std::format("failed to save image:{}", image.width).c_str());
      }
    }

    texture = to_texture(final_images);

    return true;
  }

  void unload() {
    if (texture.id != 0) {
      UnloadTexture(texture);
      texture = {};
    }
  }

private:
  ImageMips load_image_with_mips(fs::path path) {
    const std::string file_name = path.string();

    ImageMips mips{};

    mips[0] = LoadImage(file_name.c_str());
    if (!IsImageValid(mips[0])) {
      TraceLog(LOG_ERROR, "Failed to load: %s", file_name.c_str());
      return {};
    }
    ImageFormat(&mips[0], PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    // create mips
    for(int level = 1; level < MIP_LEVELS; level++) {
      const Image& previous = mips[level - 1];
      mips[level] = ImageCopy(previous);

      ImageResize(
        &mips[level],
        previous.width / 2,
        previous.height / 2
      );
    }

    return mips;
  }
  bool same_size(std::array<Image,file_names.size()> mips) {
    int height = mips[0].height;
    int width  = mips[0].width;
    int format = mips[0].format;
    for (int i = 1; i < mips.size(); i++) {
      if (mips[i].height != height) {
        TraceLog(LOG_ERROR, std::format(
          "images had incompatible height:{},{}",height, mips[i].height).c_str()
        );
        return false;
      }
      if (mips[i].width != width) {
        TraceLog(LOG_ERROR, std::format(
          "images had incompatible width:{},{}",width, mips[i].width).c_str()
        );
        return false;
      }
      if (mips[i].format != format) {
        TraceLog(LOG_ERROR, std::format(
          "images had incompatible format:{},{}",format, mips[i].format).c_str()
        );
        return false;
      }
    }
    return true;
  }

  ImageMips stitch_images(AllImageMips all_image_mips) {
    ImageMips result{};
    for (int i = 0; i < MIP_LEVELS; i++) {
      std::array<Image,file_names.size()> images;
      for (int j = 0; j < all_image_mips.size(); j++) {
        images[j] = all_image_mips[j][i];
      }
      assert(same_size(images));
      Image image = GenImageColor(images[0].width * n, images[0].height * n, BLANK);
      for (int j = 0; j < images.size(); j++) {
        int pos_x = j % n;
        int pos_y = j / n;
        ImageDrawImage(&image, images[j], pos_x * images[j].width, pos_y * images[j].height, WHITE);
      }
      result[i] = image;
    }
    return result;
  }

  Texture2D to_texture(ImageMips mips) {
    // create opengl texture
    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    // upload each mip level
    for (int level = 0; level < MIP_LEVELS; level++) {
      const Image &image = mips[level];
      glTexImage2D(
        GL_TEXTURE_2D,
        level,
        GL_RGB8,
        image.width,
        image.height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        image.data
      );
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MIP_LEVELS - 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0);

    // wrap opengl texture in raylib handle
    Texture2D texture = Texture2D{
      id,
      mips[0].width,
      mips[0].height,
      MIP_LEVELS,
      PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };

    for (Image &image : mips) {
      UnloadImage(image);
    }
    return texture;
  }

};