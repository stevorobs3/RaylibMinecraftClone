#pragma once
#include <filesystem>
#include <array>
#include <cassert>
#include <cmath>

#include "raylib.h"
#include "rlgl.h"
#include "external/glad.h"

#include "quad.h"
#include "world/block.h"

namespace fs = std::filesystem;

static constexpr int MIP_LEVELS = 5; // 64*64 ...4*4

using ImageMips = std::array<Image, MIP_LEVELS>;


// todo: pull implementation into .cpp file
struct TextureAtlas {
  Texture2D texture{};


  static constexpr std::array file_names{
    "dirt.png",
    "grass_block_top.png",
    "grass_block_side.png",
    "stone.png",
    "bedrock.png"
  };

  // convert into an n * n grid
  int n = std::ceil(std::sqrt(static_cast<float>(file_names.size())));


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
    Direction dir
  );

  bool load();
  void unload();

private:
  using AllImageMips = std::array<ImageMips, file_names.size()>;


  ImageMips load_image_with_mips(fs::path path);
  ImageMips stitch_images(AllImageMips all_image_mips);

  bool same_size(std::array<Image,file_names.size()> mips);

  Texture2D to_texture(ImageMips mips);

};