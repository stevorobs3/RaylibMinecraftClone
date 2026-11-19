#pragma once

#include "raylib.h"
#include <cstdint>

#include "vector.h"

enum class BlockType: uint8_t {
  AIR,
  GRASS,
  DIRT,
  STONE,
  BEDROCK
};

struct Block {

  //todo: handle non full blocks
  static BoundingBox bounds(Vector3Int position, [[maybe_unused]]
    BlockType block_type) noexcept {
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