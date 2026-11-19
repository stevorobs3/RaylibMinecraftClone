#pragma once
#include <cstddef>
#include <functional>

// Chunk coordinates are signed, independent of world-space float positions.
struct ChunkCoord {
  int x = 0, z = 0;
  bool operator==(const ChunkCoord&) const noexcept = default;
};
struct SectionCoord {
  ChunkCoord chunk{};
  int y = 0;
  bool operator==(const SectionCoord&) const noexcept = default;
};
constexpr int floor_div(int x, int divisor) noexcept {
  // Precondition: divisor > 0.
  const int q = x / divisor;
  return q - (x % divisor < 0 ? 1 : 0);
}
constexpr int floor_mod(int x, int divisor) noexcept {
  const int r = x % divisor;
  return r < 0 ? r + divisor : r;
}
struct ChunkCoordHash {
  std::size_t operator()(ChunkCoord c) const noexcept {
    const auto h1 = std::hash<int>{}(c.x);
    const auto h2 = std::hash<int>{}(c.z);
    return h1 ^ (h2 + 0x9e3779b9u + (h1 << 6) + (h1 >> 2));
  }
};
struct SectionCoordHash {
  std::size_t operator()(SectionCoord s) const noexcept {
    const auto h = ChunkCoordHash{}(s.chunk);
    const auto other = std::hash<int>{}(s.y);
    return h ^ (other + 0x9e3779b9u + (h << 6) + (h >> 2));
  }
};
