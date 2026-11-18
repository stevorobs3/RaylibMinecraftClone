#pragma once

#include <cmath>
#include <vector>
#include <cassert>
#include <unordered_set>

#include "raylib.h"
#include "vector.h"

#include "block.h"
#include "chunk_section.h"
#include "chunk.h"


// todo: this currently holds the responsibility of
// marking chunk sections as dirty. Not sure if this is the right
// place for that longer term

struct World {

	World();

	// explicitly disallow copying and moving
	World(const World &world) = delete;
	World(World &&world) = delete;
	World& operator=(const World &world) = delete;
	World& operator=(World &&world) = delete;


	void initialise();

  void mark_dirty(int chunk_index, int chunk_section_index);

	bool contains(Vector3Int position) const;

  BlockType get(int x, int y, int z) const;

  BlockType get(Vector3Int pos) const {
    return get(pos.x,pos.y,pos.z);
  }

  ChunkSection& get_chunk_section(
    int chunk_index,
    int chunk_section_index
  );


  void set(int x, int y, int z, BlockType block_type);

  void set(Vector3Int pos, BlockType block_type) {
    set(pos.x,pos.y,pos.z, block_type);
  }

  Vector3Int to_grid_position(Vector3 vec3) const {
    // todo: handle negative ranges too
    return Vector3Int(
      std::floor(vec3.x),
      std::floor(vec3.y),
      std::floor(vec3.z)
    );
  }

  std::vector<Vector3Int> positions_within_range(
    Vector3 origin,
    int range
  ) const;

  static constexpr int n = 2;
  static constexpr int num_chunks = n * n;

  static constexpr int height = Chunk::n_sections * ChunkSection::n;
  static constexpr int width =  ChunkSection::n * n;
  static constexpr int block_count = width * width * height;

  static constexpr BoundingBox bounds{
    {0, 0, 0},
    {width, height, width}
  };

  struct IntPairHash {
    std::size_t operator()(const std::pair<int,int> & index) const {
      auto h1 = std::hash<int>{}(index.first);
      auto h2 = std::hash<int>{}(index.second);
      return h1 ^ (h2 << 1);
    }
  };


  std::unordered_set<std::pair<int,int>,IntPairHash> dirty_chunk_sections;

  // number of chunks per x / z direction
  std::vector<Chunk> chunks;

};
