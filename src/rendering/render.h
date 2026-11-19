#pragma once
#include <vector>
#include <raylib.h>
#include "world/world.h"
#include "world_mesh.h"
#include "player/player.h"
#include "player/player_controller.h"

struct Renderer {
  Renderer(const World& world,const WorldMesh& world_mesh,
           const Player& player,const PlayerController& controller) noexcept
    : world_(world), world_mesh_(world_mesh), player_(player), controller_(controller) {}
  void draw_world() const;
  void draw_player() const;
  void draw_ui(bool show_physics_debug,const std::vector<Vector3Int>& overlapping) const;
private:
  const World& world_;
  const WorldMesh& world_mesh_;
  const Player& player_;
  const PlayerController& controller_;
  void draw_player_target() const;
};
