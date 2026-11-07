#pragma once

#include <experimental/mdspan>
#include <array>
#include <ranges>
#include <tuple>

#include "vector.h"

enum class BlockType {
		Air,
		Grass,
		Dirt
};

struct Block {
	BlockType block_type = BlockType::Air;
};


using Blocks = std::experimental::mdspan<
		Block,
		std::experimental::dextents<std::size_t, 3>
	>;

using ConstBlocks = std::experimental::mdspan<
		const Block,
		std::experimental::dextents<std::size_t, 3>
	>;


struct World {
	Blocks blocks;

	World() {
		blocks = Blocks(storage.data(), n, height, n);
		initialise();
	}

	// explicitly disallow copying and moving
	World(const World &world) = delete;
	World(World &&world) = delete;
	World& operator=(const World &world) = delete;
	World& operator=(World &&world) = delete;


	void initialise() {
		for (int x = 0; x < n; x++) {
			for (int z = 0; z < n; z++) {
				auto block_type = (x + z) % 2 == 0 ? BlockType::Grass : BlockType::Dirt;
				blocks[x,0,z] = Block(block_type);
			}
		}

		for(int i = 0; i < n; i++) {
			blocks[i,1,n-1].block_type = BlockType::Grass;
		}
	}

	bool contains(Vector3Int position) {
		return position.x >= 0 && position.x < n &&
			position.y >= 0 && position.y < height &&
			position.z >= 0 && position.z < n;
	}

	auto blocks_with_positions() const {
		return std::views::iota(0, blockCount)
				| std::views::transform([this](int i) {
						int x = i / (height * n);
						int y = (i / n) % height;
						int z = i % n;

						return std::tuple<const Block&, int, int, int>(
								blocks[x, y, z], x, y, z
						);
				});
}
	static constexpr int n = 5;
	static constexpr int height = 3;
	static constexpr int blockCount = n * height * n;
private:
	std::array<Block, blockCount> storage{};


};
