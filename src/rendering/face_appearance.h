#pragma once
#include <optional>
#include <raylib.h>
#include "quad.h"

// Texture IDs are independent of BlockType enum values.
enum class TextureId : unsigned char {
  Dirt, GrassTop, GrassSide, Stone, Bedrock, GrassSideOverlay
};
struct TextureLayer {
  TextureId texture;
  Color tint = WHITE;
};
struct FaceAppearance {
  TextureLayer base;
  std::optional<TextureLayer> overlay{};
};

// Returning an aggregate requires no allocation.
inline FaceAppearance face_appearance(BlockType block,
  Direction dir) noexcept {
  // Placeholder until biome colours
  constexpr Color grass{145, 189, 89, 255};
  switch (block) {
    case BlockType::GRASS:
      if (dir == Direction::UP)
        return {{TextureId::GrassTop, grass}, {}};
      if (dir == Direction::DOWN)
        return {{TextureId::Dirt}, {}};
      return {{TextureId::GrassSide},
              TextureLayer{TextureId::GrassSideOverlay, grass}};
    case BlockType::DIRT: return {{TextureId::Dirt}, {}};
    case BlockType::STONE: return {{TextureId::Stone}, {}};
    case BlockType::BEDROCK: return {{TextureId::Bedrock}, {}};
    case BlockType::AIR: break;
  }
  // Caller never requests a visible face for AIR.
  return {{TextureId::Dirt}, {}};
}
