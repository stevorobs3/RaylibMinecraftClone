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

void Renderer::draw_player(bool /*show_physics_debug*/) {
  DrawBoundingBox(controller.bounds(), BLACK);

  if (player.target) {
    draw_player_target();
  }
}

void Renderer::draw_ui(bool show_physics_debug) {
  DrawCircle(GetScreenWidth() / 2, GetScreenHeight() / 2, 3, BLACK);
  DrawFPS(0, 0);

  if (show_physics_debug) {
    const Vector3 normal = controller.ground_normal();
    const Vector3 velocity = controller.velocity();
    DrawText(
      TextFormat(
        "Jolt: %s | walkable: %s | vy: %.2f",
        controller.ground_state_name(),
        controller.grounded() ? "yes" : "no",
        velocity.y
      ),
      8, 35, 18, BLACK
    );
    DrawText(
      TextFormat("Ground normal: (%.2f, %.2f, %.2f)",
        normal.x, normal.y, normal.z),
      8, 58, 18, BLACK
    );

    DrawText("Last jump presses (feet Y / VY / grounded / fired):",
      8, 87, 16, BLACK);
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
        8, 108 + static_cast<int>(i) * 21, 16, BLACK
      );
    }
  }
}
