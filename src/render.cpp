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

void Renderer::draw_player(bool show_physics_debug) {
  DrawBoundingBox(controller.bounds(), BLACK);

  if (show_physics_debug) {
    // Jolt's actual collision capsule, in addition to the former AABB.
    const auto bounds = controller.bounds();
    const float radius = controller.capsule_radius();
    const Vector3 feet = controller.position();
    DrawCapsuleWires(
      {feet.x, feet.y + radius, feet.z},
      {feet.x, bounds.max.y - radius, feet.z},
      radius, 4, 12, DARKGRAY
    );
  }

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
  }
}
