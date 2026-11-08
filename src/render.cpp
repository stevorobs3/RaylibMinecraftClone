#include <raylib.h>
#include <rlgl.h>

#include "render.h"

void Renderer::draw_world() {
  DrawGrid(10, 1);
  for (auto&& [block, x, y, z] : world.blocks_with_positions()) {
    if (block.block_type != BlockType::Air) {
      const Color color = block_color.at(block.block_type);
      DrawCube(
        Vector3{x + 0.5f, y + 0.5f, z + 0.5f},
        1, 1, 1, color
      );
    }
  }
}

void Renderer::draw_player_target() {
  const auto& position = player.target->position;

  rlDrawRenderBatchActive();
  auto original_line_width = rlGetLineWidth();
  rlSetLineWidth(5.0f);

  const Vector3 centre{
    position.x + 0.5f,
    position.y + 0.5f,
    position.z + 0.5f
  };
  DrawCubeWiresV(centre, {1, 1, 1}, BLACK);

  rlDrawRenderBatchActive();
  rlSetLineWidth(original_line_width);
}

void Renderer::draw_player() {
  DrawBoundingBox(controller.bounds(), BLACK);
  if (player.target) {
    draw_player_target();
  }
}

void Renderer::draw_ui() {
  DrawCircle(GetScreenWidth() / 2, GetScreenHeight() / 2, 3, BLACK);
  DrawFPS(0, 0);
}
