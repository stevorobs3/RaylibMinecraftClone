#pragma once

#include <raylib.h>
#include <vector>


#include "world/world.h"
#include "world_mesh.h"
#include "player/player.h"
#include "player/player_controller.h"

struct DebugTimer {
  double value;
  double max;

  // todo: rebuilding a mesh takes about 40-60ms
  // can that be sped up?
  // reset every 5 seconds
  int n = 300;
  int count = 0;

  void set_value(double new_value) {
    count++;
    if (count == n) {
      count = 0;
      max = 0;
    }
    if (new_value >= max)
      max = new_value;
    value = new_value;
  }

  void operator=(double new_value) {
    set_value(new_value);
  }

  std::string to_string() {
    return std::format("{:.2f}:({:.2f})",value,max);
  }

};

struct DebugTimers {
  DebugTimer apply_look, find_target, manipulate_blocks, controller_update, mesh_rebuild, render;
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
