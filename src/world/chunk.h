#pragma once
#include <array>
#include <utility>
#include <raylib.h>
#include "chunk_section.h"

struct Chunk {
  static constexpr int n_sections = 24;
  ChunkCoord coord{};
  Vector3Int origin{};
  BoundingBox bounds{};
  std::array<ChunkSection,n_sections> chunk_sections;

  explicit Chunk(ChunkCoord coord)
    : coord(coord),
      origin{coord.x*ChunkSection::n,0,coord.z*ChunkSection::n},
      bounds{
        Vector3{static_cast<float>(origin.x),0,static_cast<float>(origin.z)},
        Vector3{static_cast<float>(origin.x+ChunkSection::n),
                static_cast<float>(n_sections*ChunkSection::n),
                static_cast<float>(origin.z+ChunkSection::n)}
      },
      chunk_sections(make_sections(coord,std::make_index_sequence<n_sections>{})) {}

  Chunk(const Chunk&) = delete;
  Chunk& operator=(const Chunk&) = delete;
  Chunk(Chunk&&) = delete;
  Chunk& operator=(Chunk&&) = delete;

private:
  template<std::size_t... I>
  static std::array<ChunkSection,n_sections>
  make_sections(ChunkCoord c, std::index_sequence<I...>) {
    return {{ChunkSection{SectionCoord{c,static_cast<int>(I)}}...}};
  }
};
