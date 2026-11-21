#include <format>
#include <raylib.h>
#include <rlgl.h>
#include <string>

#include "render.h"

void Renderer::draw_world() const {
  for (const auto& [coord, section_mesh] :
       world_mesh_.chunk_section_meshes) {
    const Vector3 origin{
      static_cast<float>(coord.chunk.x * ChunkSection::n),
      static_cast<float>(coord.y * ChunkSection::n),
      static_cast<float>(coord.chunk.z * ChunkSection::n)
    };

    for (const OwnedModel& model : section_mesh.models())
      DrawModel(model.get(), origin, 1.0f, WHITE);
  }

  // Chunk outlines (debug visualisation to be polished later).
  rlDrawRenderBatchActive();

  const float old_width = rlGetLineWidth();
  rlSetLineWidth(3.0f);

  for (const auto& [coord, chunk] : world_.chunks())
    DrawBoundingBox(chunk.bounds, BLACK);

  rlDrawRenderBatchActive();
  rlSetLineWidth(old_width);
}

void Renderer::draw_player_target() const {
  if (!player_.target)
    return;

  const auto cell = player_.target->position;

  DrawCubeWiresV(
    {cell.x + 0.5f, cell.y + 0.5f, cell.z + 0.5f},
    {1.0f, 1.0f, 1.0f},
    BLACK
  );
}

void Renderer::draw_player() const {
  DrawBoundingBox(controller_.bounds(), BLACK);
  draw_player_target();
}

void Renderer::draw_ui(
  bool show_physics_debug,
  const std::vector<Vector3Int>& overlapping
) const {
  DrawCircle(
    GetScreenWidth() / 2, GetScreenHeight() / 2, 3, BLACK
  );
  DrawFPS(0, 0);

  if (!show_physics_debug)
    return;

  const Vector3 feet     = controller_.feet;
  const Vector3 velocity = controller_.velocity;

  DrawText(
    TextFormat(
      "Feet: %.3f,%.3f,%.3f  velocity: %.2f,%.2f,%.2f",
      feet.x, feet.y, feet.z,
      velocity.x, velocity.y, velocity.z
    ),
    8, 35, 18, BLACK
  );

  std::string lines = std::format(
    "overlapping candidate cells: {}", overlapping.size()
  );

  for (const auto pos : overlapping)
    lines += std::format("\n{}", pos.to_string());

  DrawText(lines.c_str(), 8, 55, 18, BLACK);
}
