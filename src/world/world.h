#pragma once
#include <cassert>
#include <cmath>
#include <optional>
#include <unordered_map>
#include <vector>
#include <raylib.h>

#include "block.h"
#include "chunk.h"
#include "coords.h"
#include "vector.h"

enum class SetBlockResult { Changed, Unchanged, Unloaded, OutOfBounds };

struct World {
  static constexpr int initial_grid_size = 2;
  static constexpr int height = Chunk::n_sections * ChunkSection::n;
  static constexpr int chunk_size = ChunkSection::n;

  World();
  World(const World&) = delete;
  World& operator=(const World&) = delete;
  World(World&&) = delete;
  World& operator=(World&&) = delete;

  void initialise();
  bool contains(Vector3Int pos) const noexcept;
  bool is_loaded(ChunkCoord coord) const noexcept;
  Chunk* find_chunk(ChunkCoord coord) noexcept;
  const Chunk* find_chunk(ChunkCoord coord) const noexcept;
  Chunk& get_or_create_chunk(ChunkCoord coord); // may allocate
  ChunkSection* find_section(SectionCoord coord) noexcept;
  const ChunkSection* find_section(SectionCoord coord) const noexcept;

  std::optional<BlockType> try_get(Vector3Int pos) const noexcept;
  BlockType get(Vector3Int pos) const noexcept {
    auto result=try_get(pos);
    assert(result.has_value());
    return *result;
  }
  BlockType get(int x,int y,int z) const noexcept { return get({x,y,z}); }

  SetBlockResult set(Vector3Int pos,BlockType type) noexcept;
  SetBlockResult set(int x,int y,int z,BlockType type) noexcept {
    return set({x,y,z},type);
  }
  void mark_dirty(SectionCoord coord) noexcept;

  static constexpr ChunkCoord to_chunk_coord(Vector3Int pos) noexcept {
    return {floor_div(pos.x,chunk_size),floor_div(pos.z,chunk_size)};
  }
  static constexpr Vector3Int to_local_position(Vector3Int pos) noexcept {
    return {floor_mod(pos.x,chunk_size), floor_mod(pos.y,chunk_size), floor_mod(pos.z,chunk_size)};
  }
  static Vector3Int to_grid_position(Vector3 vec3) noexcept {
    return {static_cast<int>(std::floor(vec3.x)),
            static_cast<int>(std::floor(vec3.y)),
            static_cast<int>(std::floor(vec3.z))};
  }
  std::vector<Vector3Int> positions_within_range(Vector3 origin,int range) const;

  std::unordered_map<ChunkCoord,Chunk,ChunkCoordHash> chunks;
};
