#include <cfloat>
#include <raylib.h>
#include <rlgl.h>
#include <array>
#include <optional>
#include <experimental/mdspan>
#include <unordered_map>

enum class BlockType {
		Air,
		Grass,
		Dirt
};

struct Block {
	BlockType block_type = BlockType::Air;
};


constexpr int n = 5;
constexpr int height = 3;
constexpr int blockCount = n * height * n;

using Blocks = std::experimental::mdspan<
		Block,
		std::experimental::dextents<std::size_t, 3>
	>;

using ConstBlocks = std::experimental::mdspan<
		const Block,
		std::experimental::dextents<std::size_t, 3>
	>;

std::unordered_map<BlockType, Color> block_color =
{
	{BlockType::Grass, GREEN},
	{BlockType::Dirt, BROWN}
};

void DrawWorld(ConstBlocks blocks) {
	
	DrawGrid(10,1);
	for (int x = 0; x < n; x++) {
		for (int y = 0; y < height; y++) {
			for (int z = 0; z < n; z++) {
				auto& block = blocks[x,y,z];
				if (block.block_type != BlockType::Air) {
					const Color color = block_color.at(block.block_type);
					DrawCube(Vector3(x + 0.5, y + 0.5, z + 0.5), 1, 1, 1, color);
				}
			}
		}
	}
}

// basically just a vector3Int, but explicitly is the index in x,y,z of the block.
// later - size_t?
struct BlockPosition {
	int x,y,z;
};

std::optional<BlockPosition> FindTargetBlock(ConstBlocks blocks, Camera camera) {
	auto screen_center = Vector2(GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f);
	auto ray = GetScreenToWorldRay(screen_center, camera);

	float distance = FLT_MAX;

	std::optional<BlockPosition> target = std::nullopt;
	for (int x = 0; x < n; x++) {
		for (int y = 0; y < height; y++) {
			for (int z = 0; z < n; z++) {
				if (blocks[x,y,z].block_type == BlockType::Air) continue;
				
				auto bb = BoundingBox();
				bb.min = Vector3(x,y,z);
				bb.max = Vector3(x + 1,y + 1,z + 1);
				RayCollision rayCollision = GetRayCollisionBox(ray, bb);
				if (rayCollision.hit && rayCollision.distance < distance) {
					distance = rayCollision.distance;
					target = BlockPosition(x, y, z);
				}
			}
		}
	}


	return target;
}

void DrawTarget(BlockPosition position) {
	// flush pending draw calls so line thickness only affects this batch. 
	rlDrawRenderBatchActive();
	auto original_line_width = rlGetLineWidth();
	rlSetLineWidth(5.0f);

	auto cube_origin = Vector3(position.x + 0.5, position.y + 0.5, position.z + 0.5);
	DrawCubeWiresV(cube_origin, Vector3(1,1,1), BLACK);
	
	rlDrawRenderBatchActive();
	rlSetLineWidth(original_line_width);
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

	Blocks blocks(storage.data(), n, height, n);

	
	for (int x = 0; x < n; x++) {
		for (int z = 0; z < n; z++) {
			auto block_type = (x + z) % 2 == 0 ? BlockType::Grass : BlockType::Dirt;
			blocks[x,0,z] = Block(block_type);
		}
	}

	while (!WindowShouldClose()) {
		// User input
		UpdateCamera(
				&camera,
				CAMERA_FIRST_PERSON
		);

		bool should_delete_target_block = IsMouseButtonPressed(0);

		std::optional<BlockPosition> target = FindTargetBlock(blocks, camera);

		if (should_delete_target_block && target) {
			auto &t = *target;

			blocks[t.x, t.y, t.z].block_type = BlockType::Air;
			target = FindTargetBlock(blocks, camera);
		}

		BeginDrawing();
		ClearBackground(RAYWHITE);

		BeginMode3D(camera);
		
		DrawWorld(blocks);

		if (target) {
			DrawTarget(target.value());
		}

		EndMode3D();

		// Draw UI
		DrawCircle(GetScreenWidth()/2, GetScreenHeight() / 2, 3, BLACK);
		DrawFPS(0,0);

		EndDrawing();
	}

	CloseWindow();
	return 0;
}

