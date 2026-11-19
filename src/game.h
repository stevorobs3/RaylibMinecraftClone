#pragma once

#include "raylib.h"

#include "player/player.h"
#include "player/player_controller.h"
#include "world/world.h"
#include "rendering/world_mesh.h"
#include "rendering/texture_atlas.h"
#include "rendering/render.h"
#include "world_physics.h"
#include "fixed_step.h"

struct Game {
  TextureAtlas texture_atlas{};
  World world{};
  Player player{};
  Vector3 player_initial_position;
  Vector3 player_size;
  Camera3D camera;
  WorldPhysics physics;
  PlayerController controller;
  WorldMesh world_mesh;
  Renderer renderer;
  FixedStepper fixed_stepper{};

  Game();

  int run();
};