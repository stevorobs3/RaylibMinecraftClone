#pragma once

#include "raylib.h"
#include <cstdint>

#include "vector.h"

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

};