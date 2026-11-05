#include <raylib.h>
#include <array>
#include <experimental/mdspan>
#include <unordered_map>

enum class BlockType {
		Air,
		Grass,
		Dirt
};

struct Block {
	BlockType block_type;
};

const int n = 5;
constexpr int blockCount = n * n * n;

using Blocks = std::experimental::mdspan<
		Block,
		std::experimental::dextents<std::size_t, 3>
	>;

std::unordered_map<BlockType, Color> block_color =
{
	{BlockType::Grass, GREEN},
	{BlockType::Dirt, BROWN}
};

void DrawWorld(Blocks &blocks) {
	
	DrawGrid(10,1);
	for (int x = 0; x < n; x++) {
		for (int y = 0; y < n; y++) {
			for (int z = 0; z < n; z++) {
				auto block = blocks[x,y,z];
				if (block.block_type != BlockType::Air) {
					auto color = block_color[block.block_type];
					DrawCube(Vector3(x + 0.5, y + 0.5, z + 0.5), 1, 1, 1, color);
				}
			}
		}
	}

}

int main() {
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(1280, 720, "Raylib minecraft clone");
	DisableCursor();
	PollInputEvents();

	Camera3D camera{
		.position = {-2.0f, 4.0f, -2.0f},
		.target = {2.5f, 0, 2.5f},
		.up = {0.0f, 1.0f, 0.0f},
		.fovy = 45.0f,
		.projection = CAMERA_PERSPECTIVE
	};
		

	SetTargetFPS(60);


	std::array<Block, blockCount> storage{};

	Blocks blocks(storage.data(), n, n, n);

	
	for (int x = 0; x < n; x++) {
		for (int y = 0; y < n; y++) {
			for (int z = 0; z < n; z++) {
				if (y != 0) {
					blocks[x,y,z] = Block(BlockType::Air);
				}
				else {
					auto block_type = (x + z) % 2 == 0 ? BlockType::Grass : BlockType::Dirt;
					blocks[x,y,z] = Block(block_type);
				}
			}
		}
		
	}

	while (!WindowShouldClose()) {
		// User input
		UpdateCamera(
				&camera,
				CAMERA_FIRST_PERSON
		);

		BeginDrawing();
		ClearBackground(RAYWHITE);
		
		BeginMode3D(camera);
		
		DrawWorld(blocks);

		EndMode3D();
		EndDrawing();
	}

	CloseWindow();
	return 0;
}

