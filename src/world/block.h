#pragma once

#include "raylib.h"
#include <cstdint>

#include "vector.h"
#include "face.h"

// the order of these is tied to the textures_lookup in texture_atlas
enum class BlockType: uint8_t {
  AIR,
  GRASS,
  DIRT,
  STONE,
  BEDROCK
};

struct Block {

  //todo: handle non full blocks
  static BoundingBox bounds(Vector3Int position, BlockType block_type) {
    return BoundingBox{
      .min = {
        static_cast<float>(position.x),
        static_cast<float>(position.y),
        static_cast<float>(position.z)
      },
      .max = {
        static_cast<float>(position.x + 1),
        static_cast<float>(position.y + 1),
        static_cast<float>(position.z + 1)
      }
    };
  }

  static constexpr Color color(BlockType block_type, Face::Direction direction) {
    if (direction == Face::Direction::UP && block_type == BlockType::GRASS) {
      // todo: not right colour, look this up in grass block
      return Color(145, 189, 89,255);
    }
    else {
      return WHITE;
    }
  }

};