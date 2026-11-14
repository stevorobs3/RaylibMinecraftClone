#pragma once

#include <raylib.h>
#include <unordered_map>
#include <vector>


#include "world.h"
#include "world_mesh.h"
#include "player.h"
#include "player_controller.h"

struct DebugTimers {
  double apply_look, find_target, manipulate_blocks, controller_update, render;
};


struct Renderer {
  Renderer(
    const World& world,
    const WorldMesh& world_mesh,
    const Player& player,
    const PlayerController& controller
  ) : world(world), world_mesh(world_mesh), player(player), controller(controller) {}

  void draw_world();
  void draw_player();
  void draw_ui(bool show_physics_debug, DebugTimers debug_timers, std::vector<Vector3Int> blocks_overlapping_player);

  Mesh GenerateMesh(ChunkSection chunk_section);

private:
  const World& world;
  const Player& player;
  const PlayerController& controller;
  const WorldMesh& world_mesh;


  void draw_player_target();
};
