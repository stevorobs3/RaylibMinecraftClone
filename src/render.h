#pragma once

#include <unordered_map>

#include "world.h"
#include "raylib.h"
#include "player.h"
#include "physics/physics.h"

struct Renderer {
  Renderer(
    const World& world,
    const Player& player,
    const PlayerController& controller
  ) : world(world), player(player), controller(controller) {}

  void draw_world();
  void draw_player();
  void draw_ui();

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
