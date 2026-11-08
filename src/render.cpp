#include <raylib.h>
#include <rlgl.h>

#include "render.h"

void Renderer::draw_world() {
	DrawGrid(10,1);
	for (auto && [block, x, y, z] : world.blocks_with_positions()) {
		if (block.block_type != BlockType::Air) {
			const Color color = block_color.at(block.block_type);
			DrawCube(Vector3(x + 0.5, y + 0.5, z + 0.5), 1, 1, 1, color);
		}
	}
}

void Renderer::draw_player_target() {
	auto &position = player.target->position;
	// flush pending draw calls so line thickness only affects this batch. 
	rlDrawRenderBatchActive();
	auto original_line_width = rlGetLineWidth();
	rlSetLineWidth(5.0f);

	auto cube_center = Vector3(position.x + 0.5, position.y + 0.5, position.z + 0.5);
	DrawCubeWiresV(cube_center, Vector3(1,1,1), BLACK);
	
	rlDrawRenderBatchActive();
	rlSetLineWidth(original_line_width);
}

void Renderer::draw_player() {
	DrawBoundingBox(player.get_bounds(), BLACK);
	if (player.target) {
		draw_player_target();
	}
}

void Renderer::draw_ui() {
	DrawCircle(GetScreenWidth()/2, GetScreenHeight() / 2, 3, BLACK);
	DrawFPS(0,0);
}