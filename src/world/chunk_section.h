#pragma once

#include <experimental/mdspan>
#include <cassert>

#include "raylib.h"
#include "raymath.h"

#include "block.h"
#include "quad.h"

struct World;

using Blocks = std::experimental::mdspan<
    BlockType,
    std::experimental::dextents<std::size_t, 3>
  >;

using ConstBlocks = std::experimental::mdspan<
    const BlockType,
    std::experimental::dextents<std::size_t, 3>
  >;

struct ChunkSection {

  World& world;
  Vector3 origin;
  BoundingBox bounds;
  static constexpr int n = 16;
  static constexpr int blockCount = n * n * n;

  explicit ChunkSection(World& world, Vector3 origin)
    : world(world), origin(origin),
      bounds{
        origin,
        origin + Vector3(n, n, n)
      } {}

  // todo: explicitly disallow copying and moving
  // ChunkSection(const ChunkSection &chunk_section) = delete;
  // ChunkSection(ChunkSection &&chunk_section) = delete;
  // ChunkSection& operator=(const ChunkSection &chunk_section) = delete;
  // ChunkSection& operator=(ChunkSection &&chunk_section) = delete;


  Blocks blocks() {
    return Blocks(storage.data(), n, n, n);
  }

  ConstBlocks const_blocks() const {
    return ConstBlocks(storage.data(), n, n, n);
  }

  bool fill(BlockType block_type) {
    bool dirty = false;
    for (BlockType & stored_type : storage) {
      if (stored_type != block_type)
        dirty = true;
    }
    storage.fill(block_type);
    return dirty;
  }

  bool contains(Vector3Int position) const {
    return position.x >= 0 && position.x < ChunkSection::n &&
      position.y >= 0 && position.y < ChunkSection::n &&
      position.z >= 0 && position.z < ChunkSection::n;
  }

  BlockType internal_neighbour(Direction dir, Vector3Int local_position) const {
    assert(contains(local_position));
    auto neighbour_pos = local_position + Quad::lookup_normal(dir);
    assert(contains(neighbour_pos));
    return const_blocks()[neighbour_pos.x,neighbour_pos.y,neighbour_pos.z];
  }

private:
  // todo: packedArray based on palette
  std::array<BlockType, blockCount> storage{};

};

