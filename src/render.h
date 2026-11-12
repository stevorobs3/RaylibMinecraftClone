#pragma once

#include <raylib.h>
#include <unordered_map>
#include <vector>


#include "world.h"
#include "player.h"
#include "player_controller.h"

struct DebugTimers {
  double apply_look, find_target, manipulate_blocks, controller_update, render;
};
  

struct Renderer {
  Renderer(
    const World& world,
    const Player& player,
    const PlayerController& controller
  ) : world(world), player(player), controller(controller) {}

  void draw_world();
  void draw_player();
  void draw_ui(bool show_physics_debug, DebugTimers debug_timers, std::vector<Vector3Int> blocks_overlapping_player);

private:
  const World& world;
  const Player& player;
  const PlayerController& controller;

  const std::unordered_map<BlockType, Color> block_color{
    {BlockType::Grass, GREEN},
    {BlockType::Dirt, BROWN}
  };

  void draw_player_target();
};
