#pragma once

#include <experimental/mdspan>
#include <array>
#include <ranges>
#include <tuple>
#include <iostream>
#include <format>

#include "raylib.h"
#include "vector.h"



enum class BlockType {
		Air,
		Grass,
		Dirt
};

using Blocks = std::experimental::mdspan<
    BlockType,
    std::experimental::dextents<std::size_t, 3>
  >;

using ConstBlocks = std::experimental::mdspan<
    const BlockType,
    std::experimental::dextents<std::size_t, 3>
  >;


struct ChunkSection {

  Blocks blocks() {
    return Blocks(storage.data(), n, n, n);
  }

  ConstBlocks const_blocks() const {
    return ConstBlocks(storage.data(), n, n, n);
  }

  void fill(BlockType block_type) {
    storage.fill(block_type);
  }

  static constexpr int n = 16;
  static constexpr int blockCount = n * n * n;
  
private:
  // todo: packedArray based on palette
  std::array<BlockType, blockCount> storage{};

};

struct Chunk {

  void fill_section(BlockType block_type, int i) {
    chunk_sections[i].fill(block_type);
  }
  void fill(BlockType block_type) {
    for (auto & chunk_section : chunk_sections) {
      chunk_section.fill(block_type);
    }
  }

  static constexpr int n_sections = 4;
  std::array<ChunkSection, n_sections> chunk_sections{};

};



struct World {
  
	World() {
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
    
      auto blocks = chunk.chunk_sections[0].blocks();
      for (int x = 0; x < ChunkSection::n; x++) {
        for (int z = 0; z < ChunkSection::n; z++) {
          auto block_type = (x + z) % 2 == 0 ? BlockType::Grass : BlockType::Dirt;
          blocks[x,0,z] = block_type;
        }
      }

      // chunk.fill_section(BlockType::Grass, 1);
    }    
	}

	bool contains(Vector3Int position) {
    int max_xz = n * ChunkSection::n;
    int max_y = Chunk::n_sections * ChunkSection::n;

		return position.x >= 0 && position.x < max_xz &&
			position.y >= 0 && position.y < max_y &&
			position.z >= 0 && position.z < max_xz;
	}

  // bounds of the world
  BoundingBox bounds() {
    return BoundingBox(
      {0, 0, 0},
      {width, height, width}
    );
  }

  BlockType get(int x, int y, int z) const {
    int index = (x / ChunkSection::n) + n * (z / ChunkSection::n) ;
    
    int chunk_section_index = y / ChunkSection::n;
    int section_x = x % ChunkSection::n;
    int section_y = y % ChunkSection::n;
    int section_z = z % ChunkSection::n;
    // std::cout << std::format("section x,y,z: {},{},{}",section_x,section_y,section_z) << std::endl;
    return chunks[index]
      .chunk_sections[chunk_section_index]
      .const_blocks()[section_x, section_y, section_z]; 
  }

  BlockType get(Vector3Int pos) const {
    return get(pos.x,pos.y,pos.z);
  }

  void set(int x, int y, int z, BlockType block_type) {
    int index = (x / ChunkSection::n) + n * (z / ChunkSection::n) ;
    
    int chunk_section_index = y / ChunkSection::n;
    int section_x = x % ChunkSection::n;
    int section_y = y % ChunkSection::n;
    int section_z = z % ChunkSection::n;
    // std::cout << std::format("set block, {},{},{} {},{},{} {} {}",x,y,z,section_x,section_y,section_z, index, chunk_section_index) << std::endl;
    chunks[index]
      .chunk_sections[chunk_section_index]
      .blocks()[section_x, section_y, section_z] = block_type; 
  }

  // todo: remove this - its terrible to use now!!!
	auto block_types_with_positions() const {
		return std::views::iota(0, block_count)
        | std::views::transform([this](int i) {
						int y = i / (width * width);
						int z = (i % (width * width)) / width;
						int x = (i % (width * width)) % width;
            // std::cout << std::format("x,y,z= {},{},{}",x,y,z) << std::endl;
            return std::tuple<BlockType, int, int, int>(
								get(x, y, z), x, y, z
						);
				});
  }
  static constexpr int n = 2;
  static constexpr int num_chunks = n * n;
  
  static constexpr int height = Chunk::n_sections * ChunkSection::n;
  static constexpr int width =  ChunkSection::n * n;
  static constexpr int block_count = width * width * height; 
  
private:
  // number of chunks per x / z direction
  std::array<Chunk, n * n> chunks{};



};
