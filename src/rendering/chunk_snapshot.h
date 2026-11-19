#pragma once
#include <array>
#include <cassert>
#include "world/world.h"

// Immutable input to CPU meshing. A one-cell halo contains neighbours
// including other sections/chunks; missing world cells are treated as air.
struct ChunkSnapshot {
  static constexpr int side = ChunkSection::n+2;
  SectionCoord coord{};
  Vector3Int origin{};
  std::array<BlockType,side*side*side> data{};

  static constexpr std::size_t index(int x,int y,int z) noexcept {
    return static_cast<std::size_t>(((x+1)*side + y+1)*side + z+1);
  }
  BlockType at(int x,int y,int z) const noexcept {
    assert(x>=-1 && x<=ChunkSection::n &&
           y>=-1 && y<=ChunkSection::n &&
           z>=-1 && z<=ChunkSection::n);
    return data[index(x,y,z)];
  }
};
inline ChunkSnapshot make_snapshot(const World& world,const ChunkSection& section) {
  ChunkSnapshot snapshot{};
  snapshot.coord=section.coord;
  snapshot.origin=section.origin;
  for(int x=-1;x<=ChunkSection::n;++x)
    for(int y=-1;y<=ChunkSection::n;++y)
      for(int z=-1;z<=ChunkSection::n;++z) {
        Vector3Int global{section.origin.x+x,section.origin.y+y,section.origin.z+z};
        snapshot.data[ChunkSnapshot::index(x,y,z)] =
          world.try_get(global).value_or(BlockType::AIR);
      }
  return snapshot;
}
