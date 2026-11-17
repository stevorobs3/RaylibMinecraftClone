#pragma once

#include <cmath>
#include <experimental/mdspan>
#include <array>
#include <vector>
#include <tuple>
#include <cassert>
#include <unordered_set>

#include "raylib.h"
#include "raymath.h"
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

using Blocks = std::experimental::mdspan<
    BlockType,
    std::experimental::dextents<std::size_t, 3>
  >;

using ConstBlocks = std::experimental::mdspan<
    const BlockType,
    std::experimental::dextents<std::size_t, 3>
  >;

// explicit circular dependency between both ChunkSection, Chunk and world
struct World;

// todo: possibly pull this into a separate file
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

  BlockType internal_neighbour(Face::Direction dir, Vector3Int local_position) const {
    assert(contains(local_position));
    auto neighbour_pos = local_position + Face::normal(dir);
    assert(contains(neighbour_pos));
    return const_blocks()[neighbour_pos.x,neighbour_pos.y,neighbour_pos.z];
  }

private:
  // todo: packedArray based on palette
  std::array<BlockType, blockCount> storage{};

};

// todo: separate file
struct Chunk {

  World &world;
  Vector3 origin;
  BoundingBox bounds;


  Chunk(World& world, Vector3 origin) : world(world), origin(origin) {
    chunk_sections.reserve(n_sections);
    for (int i = 0; i < n_sections; i++) {
      chunk_sections.emplace_back(
        world, origin + Vector3(0, ChunkSection::n * i, 0)
      );
    }
    bounds = {
      origin,
      origin + Vector3(ChunkSection::n, n_sections*ChunkSection::n, ChunkSection::n)
    };
  }

  // todo: explicitly disallow copying and moving
  // Chunk(const Chunk &chunk) = delete;
  // Chunk(Chunk &&chunk) = delete;
  // Chunk& operator=(const Chunk &chunk) = delete;
  // Chunk& operator=(Chunk &&chunk) = delete;

  // returns the indexes of the sections that are now dirty
  std::vector<int> fill(BlockType block_type) {
    std::vector<int> dirty{};
    for (int i = 0; i < chunk_sections.size(); i++) {
      ChunkSection & chunk_section = chunk_sections[i];
      if (chunk_section.fill(block_type)) {
        dirty.push_back(i);
      }
    }
    return dirty;
  }

  static constexpr int n_sections = 4;
  std::vector<ChunkSection> chunk_sections;

};


// todo: this currently holds the responsibility of
// marking chunk sections as dirty. Not sure if this is the right
// place for that longer term

// todo: this is getting big - probably worth a world.cpp soon
struct World {

	World() {
    chunks.reserve(n * n);
    for (int z = 0; z < n; z++) {
      for (int x = 0; x < n; x++) {
        chunks.emplace_back(*this, Vector3{
          static_cast<float>(x * ChunkSection::n),
          0,
          static_cast<float>(z * ChunkSection::n),
        });
      }
    }
    //todo: external initialisation / loading / unloading from disk
		initialise();
	}

	// explicitly disallow copying and moving
	World(const World &world) = delete;
	World(World &&world) = delete;
	World& operator=(const World &world) = delete;
	World& operator=(World &&world) = delete;


	void initialise() {
    for(int i = 0; i < chunks.size(); i++) {
      Chunk &chunk = chunks[i];

      // Reset player-edited blocks too.
      std::vector<int> dirty = chunk.fill(BlockType::AIR);
      for (int section : dirty) {
        mark_dirty(i, section);
      }

      auto blocks = chunk.chunk_sections[0].blocks();
      for (int x = 0; x < ChunkSection::n; x++) {
        for (int z = 0; z < ChunkSection::n; z++) {
          blocks[x,0,z] = BlockType::BEDROCK;
          blocks[x,1,z] = BlockType::STONE;
          blocks[x,2,z] = BlockType::STONE;

          blocks[x,3,z] = BlockType::DIRT;
          blocks[x,4,z] = BlockType::DIRT;
          blocks[x,5,z] = BlockType::DIRT;

          blocks[x,6,z] = BlockType::GRASS;
          mark_dirty(i, 0);
        }
      }

    }

	}

  void mark_dirty(int chunk_index, int chunk_section_index) {
    assert(chunk_index >= 0 && chunk_index < chunks.size());
    assert(chunk_section_index >= 0 &&  chunk_section_index < Chunk::n_sections);

    dirty_chunk_sections.emplace(chunk_index, chunk_section_index);
  }

	bool contains(Vector3Int position) const {
		return position.x >= 0 && position.x < width &&
			position.y >= 0 && position.y < height &&
			position.z >= 0 && position.z < width;
	}

  BlockType get(int x, int y, int z) const {
    assert(contains({x,y,z}));
    // todo: use global and local coords with a mdspan wrapper here
    int index = (x / ChunkSection::n) + n * (z / ChunkSection::n);

    int chunk_section_index = y / ChunkSection::n;
    int section_x = x % ChunkSection::n;
    int section_y = y % ChunkSection::n;
    int section_z = z % ChunkSection::n;
    return chunks[index]
      .chunk_sections[chunk_section_index]
      .const_blocks()[section_x, section_y, section_z];
  }

  ChunkSection& get_chunk_section(int chunk_index,int chunk_section_index) {
    assert(chunk_index >= 0 && chunk_index < chunks.size());
    assert(chunk_section_index >= 0 && chunk_section_index < Chunk::n_sections);
    return chunks[chunk_index].chunk_sections[chunk_section_index];
  }

  BlockType get(Vector3Int pos) const {
    return get(pos.x,pos.y,pos.z);
  }

  void set(int x, int y, int z, BlockType block_type) {
    assert(contains({x,y,z}));
    // avoid unneccessary work
    assert(get(x,y,z) != block_type);
    int index = (x / ChunkSection::n) + n * (z / ChunkSection::n);
    assert(index >= 0 && index < chunks.size());
    int chunk_section_index = y / ChunkSection::n;
    assert(chunk_section_index >= 0 && chunk_section_index < Chunk::n_sections);
    int section_x = x % ChunkSection::n;
    int section_y = y % ChunkSection::n;
    int section_z = z % ChunkSection::n;
    chunks[index]
      .chunk_sections[chunk_section_index]
      .blocks()[section_x, section_y, section_z] = block_type;

    mark_dirty(index, chunk_section_index);

    // todo: pull into separate place - its handy
    // todo: this is messy as
    int chunk_x = (x / ChunkSection::n);
    int chunk_z = (z / ChunkSection::n);
    if (section_x == 0) {
      int neighbour_index = (chunk_x - 1) + n * chunk_z;
      if (chunk_x > 0)
        mark_dirty(neighbour_index, chunk_section_index);
    }
    if (section_x == ChunkSection::n - 1) {
      int neighbour_index = (chunk_x + 1) + n * chunk_z;
      if (chunk_x + 1 < n)
        mark_dirty(neighbour_index, chunk_section_index);
    }
    if (section_z == 0) {
      int neighbour_index = chunk_x + n * (chunk_z - 1);
      if (chunk_z > 0)
        mark_dirty(neighbour_index, chunk_section_index);
    }
    if (section_z == ChunkSection::n - 1) {
      int neighbour_index = chunk_x + n * (chunk_z + 1);
      if (chunk_z + 1 < n)
        mark_dirty(neighbour_index, chunk_section_index);
    }
    if (section_y == 0) {
      int neighbour_index = chunk_section_index - 1;
      if (neighbour_index >= 0)
        mark_dirty(index, neighbour_index);
    }
    if (section_y == ChunkSection::n - 1) {
      int neighbour_index = chunk_section_index + 1;
      if (neighbour_index < Chunk::n_sections)
        mark_dirty(index, neighbour_index);
    }

  }

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

  std::vector<Vector3Int> positions_within_range(Vector3 origin, int range) const {

    Vector3Int origin_grid_position = to_grid_position(origin);

    std::vector<Vector3Int> positions{};
    for (int dx = -range; dx < range; dx++) {
      for (int dy = -range; dy < range; dy++) {
        for (int dz = -range; dz < range; dz++) {
          Vector3Int position = origin_grid_position + Vector3Int(dx, dy, dz);
          if (contains(position)) {
            positions.push_back(position);
          }
        }
      }
    }
    return positions;
  }

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
