#pragma once
#include <array>
#include <cassert>
#include <cstdint>
#include <experimental/mdspan>
#include <raylib.h>

#include "block.h"
#include "coords.h"

using ConstBlocks = std::experimental::mdspan<
  const BlockType, std::experimental::dextents<std::size_t, 3>
>;

struct ChunkSection {
  static constexpr int n = 16;
  static constexpr int blockCount = n*n*n;
  SectionCoord coord{};
  Vector3Int origin{};
  BoundingBox bounds{};

  explicit ChunkSection(SectionCoord coord) noexcept
    : coord(coord),
      origin{coord.chunk.x*n, coord.y*n, coord.chunk.z*n},
      bounds{
        Vector3{static_cast<float>(origin.x), static_cast<float>(origin.y), static_cast<float>(origin.z)},
        Vector3{static_cast<float>(origin.x+n), static_cast<float>(origin.y+n), static_cast<float>(origin.z+n)}
      } {}

  ChunkSection(const ChunkSection&) = delete;
  ChunkSection& operator=(const ChunkSection&) = delete;
  ChunkSection(ChunkSection&&) = delete;
  ChunkSection& operator=(ChunkSection&&) = delete;

  ConstBlocks const_blocks() const noexcept {
    return ConstBlocks(storage.data(), n,n,n);
  }
  bool contains(Vector3Int p) const noexcept {
    return p.x>=0 && p.x<n && p.y>=0 && p.y<n && p.z>=0 && p.z<n;
  }
  BlockType get_local(Vector3Int p) const noexcept {
    assert(contains(p));
    return const_blocks()[p.x,p.y,p.z];
  }
  // Mutations only via World, which also invalidates neighbour geometry.
private:
  friend struct World;
  bool set_local(Vector3Int p, BlockType type) noexcept {
    assert(contains(p));
    const std::size_t i = (static_cast<std::size_t>(p.x)*n + p.y)*n + p.z;
    if (storage[i] == type) return false;
    storage[i] = type;
    mark_dirty();
    return true;
  }
  bool fill(BlockType type) noexcept {
    bool changed = false;
    for (auto& block : storage) {
      if (block != type) { block=type; changed=true; }
    }
    if (changed) mark_dirty();
    return changed;
  }
public:
  void mark_dirty() noexcept { dirty = true; ++revision; }
  void acknowledge(std::uint64_t version) noexcept {
    if (revision == version) dirty = false;
  }
  bool dirty = true;
  std::uint64_t revision = 1;

private:
  std::array<BlockType,blockCount> storage{};
};
