#include <raylib.h>
#include <rlgl.h>

#include "render.h"


// todo: use meches instead of raw cubes here!
void Renderer::draw_world() {
  DrawGrid(10, 1);
  for (auto&& [block, x, y, z] : world.blocks_with_positions()) {
    if (block.block_type != BlockType::Air) {
      const Color color = block_color.at(block.block_type);
      DrawCube(
        {x + 0.5f, y + 0.5f, z + 0.5f},
        1, 1, 1, color
      );
    }
  }
}

void Renderer::draw_player_target() {
  const auto& cell = player.target->position;

  rlDrawRenderBatchActive();
  const auto original_line_width = rlGetLineWidth();
  rlSetLineWidth(5.0f);
  DrawCubeWiresV(
    {cell.x + 0.5f, cell.y + 0.5f, cell.z + 0.5f},
    {1.0f, 1.0f, 1.0f},
    BLACK
  );
  rlDrawRenderBatchActive();
  rlSetLineWidth(original_line_width);
}

void Renderer::draw_player() {
  DrawBoundingBox(controller.bounds(), BLACK);
  if (player.target) {
    draw_player_target();
  }
}

void Renderer::draw_ui(bool show_physics_debug) {
  DrawCircle(GetScreenWidth() / 2, GetScreenHeight() / 2, 3, BLACK);
  DrawFPS(0, 0);

  if (!show_physics_debug) {
    return;
  }

  const Vector3 feet = controller.position();
  const Vector3 velocity = controller.velocity();
  DrawText(
    TextFormat(
      "Voxel grounded: %s | Feet Y: %.3f | Vy: %.2f",
      controller.grounded() ? "yes" : "no",
      feet.y, velocity.y
    ),
    8, 35, 18, BLACK
  );

  DrawText("Last jump presses (feet Y / VY / grounded / fired):",
    8, 66, 16, BLACK);
  const auto history = controller.recent_jump_presses();
  for (std::size_t i = 0; i < controller.jump_press_count(); ++i) {
    const auto& press = history[i];
    DrawText(
      TextFormat(
        "%d: Y %.3f | Vy %.2f | %s | %s",
        static_cast<int>(i + 1),
        press.feet_y,
        press.vertical_velocity,
        press.grounded ? "ground" : "air",
        press.executed ? "fired" : "not fired"
      ),
      8, 87 + static_cast<int>(i) * 21, 16, BLACK
    );
  }
}
