#include <raylib.h>
#include <rlgl.h>

#include "render.h"
#include "world.h"


void Renderer::draw_world() {

  for (const ChunkSectionMesh& chunk_section_mesh : world_mesh.chunk_section_meshes) {
    if (chunk_section_mesh.model) {
      DrawModel(
        *chunk_section_mesh.model,
        chunk_section_mesh.chunk_section.origin,
        1.0f,
        WHITE
      );
    }
  }

  // draw chunk bounds

  // todo: have option to show / hide chunk boundaries
  // todo: use a grid thing to visually show the chunk boundaries
  rlDrawRenderBatchActive();
  const auto original_line_width = rlGetLineWidth();
  rlSetLineWidth(5.0f);

  for (const Chunk& chunk : world.chunks) {
    DrawBoundingBox(chunk.bounds, BLACK);
  }
  rlDrawRenderBatchActive();
  rlSetLineWidth(original_line_width);

  for (const Chunk& chunk : world.chunks) {
    for (const ChunkSection& chunk_section: chunk.chunk_sections) {
      DrawBoundingBox(chunk_section.bounds, BLUE);
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

void Renderer::draw_ui(
  bool show_physics_debug,
  DebugTimers debug_timers,
  std::vector<Vector3Int> blocks_ovelapping_player
) {
  DrawCircle(GetScreenWidth() / 2, GetScreenHeight() / 2, 3, BLACK);
  DrawFPS(0, 0);

  if (!show_physics_debug) {
    return;
  }

  Vector3 feet = controller.feet;
  Vector3 velocity = controller.velocity;
  DrawText(
    TextFormat(
      "Feet: %.3f,%.3f,%.3f",
      feet.x, feet.y, feet.z
    ),
    8, 35, 18, BLACK
  );

  DrawText(
    TextFormat(
      "timers: \napply_look:%.1f\nfind_target:%.1f\nmanipulate_blocks:%.1f\ncontroller_update:%.1f\nrender:%.1f",
      debug_timers.apply_look, debug_timers.find_target, debug_timers.manipulate_blocks, debug_timers.controller_update, debug_timers.render
    ),
    8, 55, 18, BLACK
  );

  std::string s = std::format("blocks:{}", blocks_ovelapping_player.size());
  for (auto block : blocks_ovelapping_player) {
    s += std::format("\n{},{},{}",block.x,block.y,block.z);
  }

  DrawText(
    s.c_str(),
    250, 35, 18, BLACK
  );
}
