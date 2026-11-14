#pragma once

#include <cmath>
#include <experimental/mdspan>
#include <array>
#include <vector>
#include <ranges>
#include <tuple>
#include <cassert>

#include "raylib.h"
#include "raymath.h"
#include "vector.h"
#include "face.h"



enum class BlockType: uint8_t {
		Air,
		Grass,
		Dirt
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

  void fill(BlockType block_type) {
    storage.fill(block_type);
  }

  bool contains(Vector3Int position) const {
    return position.x >= 0 && position.x < ChunkSection::n &&
      position.y >= 0 && position.y < ChunkSection::n &&
      position.z >= 0 && position.z < ChunkSection::n;
  }

  // todo: add similar asserts throughout the codebase!
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

  Chunk(World& world, Vector3 origin) : world(world), origin(origin) {
    chunk_sections.reserve(n_sections);
    for (int i = 0; i < n_sections; i++) {
      chunk_sections.emplace_back(
        world, origin + Vector3(0, ChunkSection::n * i, 0)
      );
    }
  }

  // todo: explicitly disallow copying and moving
  // Chunk(const Chunk &chunk) = delete;
  // Chunk(Chunk &&chunk) = delete;
  // Chunk& operator=(const Chunk &chunk) = delete;
  // Chunk& operator=(Chunk &&chunk) = delete;

  void fill_section(BlockType block_type, int i) {
    chunk_sections[i].fill(block_type);
  }
  void fill(BlockType block_type) {
    for (auto & chunk_section : chunk_sections) {
      chunk_section.fill(block_type);
    }
  }

  //todo: can be assigned on creation
  BoundingBox bounds() const {
    return BoundingBox{
      origin,
      origin + Vector3(ChunkSection::n, n_sections*ChunkSection::n, ChunkSection::n)
    };
  }

  static constexpr int n_sections = 4;
  std::vector<ChunkSection> chunk_sections;

private:
  Vector3 origin;

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
    for(Chunk &chunk : chunks) {
      chunk.fill(BlockType::Air); // Reset player-edited blocks too.

      // 1024 faces??? 32*32*1 = 1024
      // should have 1024 + 32*4 = 1152 (don't render bottom of world)
      for (int i = 0; i < chunk.n_sections; i++){
        auto blocks = chunk.chunk_sections[i].blocks();
        for (int x = 0; x < ChunkSection::n; x++) {
          for (int z = 0; z < ChunkSection::n; z++) {
            auto block_type = (x + z) % 2 == 0 ? BlockType::Grass : BlockType::Dirt;
            blocks[x,0,z] = block_type;
          }
        }
      }

      // chunk.fill_section(BlockType::Grass, 1);
    }

	}

	bool contains(Vector3Int position) const {
		return position.x >= 0 && position.x < width &&
			position.y >= 0 && position.y < height &&
			position.z >= 0 && position.z < width;
	}



  BlockType get(int x, int y, int z) const {
    // todo: assert that x,y,z is valid
    // todo: use global and local coords with a mdspan wrapper here
    int index = (x / ChunkSection::n) + n * (z / ChunkSection::n) ;

    int chunk_section_index = y / ChunkSection::n;
    int section_x = x % ChunkSection::n;
    int section_y = y % ChunkSection::n;
    int section_z = z % ChunkSection::n;
    return chunks[index]
      .chunk_sections[chunk_section_index]
      .const_blocks()[section_x, section_y, section_z];
  }

  ChunkSection& get_chunk_section(int chunk_index,int chunk_section_index) {
    // todo: assert chunk_index and chunk_section_index are valid
    return chunks[chunk_index].chunk_sections[chunk_section_index];
  }

  BlockType get(Vector3Int pos) const {
    return get(pos.x,pos.y,pos.z);
  }

  void set(int x, int y, int z, BlockType block_type) {
    //todo: assert x,y,z are valid
    int index = (x / ChunkSection::n) + n * (z / ChunkSection::n) ;

    int chunk_section_index = y / ChunkSection::n;
    int section_x = x % ChunkSection::n;
    int section_y = y % ChunkSection::n;
    int section_z = z % ChunkSection::n;

    // todo: assert index, chunk_section_index are valid
    // todo: use get_chunk_section?
    chunks[index]
      .chunk_sections[chunk_section_index]
      .blocks()[section_x, section_y, section_z] = block_type;

    // todo: only mark as dirty if block_type actually changes
    // todo: if on boundary - need to also set neighbouring non-empty chunks dirty
    dirty_chunk_sections.push_back({index, chunk_section_index});
  }

  void set(Vector3Int pos, BlockType block_type) {
    set(pos.x,pos.y,pos.z, block_type);
  }

  Vector3Int to_grid_position(Vector3 position) const {
    //todo: assert valid position
    // todo: handle negative ranges too
    return Vector3Int(
      std::floor(position.x),
      std::floor(position.y),
      std::floor(position.z)
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


  std::vector<std::tuple<int, int>> dirty_chunk_sections;

  // number of chunks per x / z direction
  std::vector<Chunk> chunks;

};
