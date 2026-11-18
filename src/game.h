#include "raylib.h"
#include "raymath.h"


#include "timer.h"
#include "player/player.h"
#include "player_controller.h"
#include "player_input.h"
#include "texture_atlas.h"
#include "world.h"
#include "world_physics.h"
#include "render.h"
#include "world_mesh.h"

struct Game {

  int run() {
    SetTraceLogLevel(LOG_WARNING);  // Suppress raylib INFO logs inside timed GPU calls.
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "Raylib minecraft clone");
    DisableCursor();
    PollInputEvents();


    TextureAtlas texture_atlas;
    World world;
    Player player;

    if (!texture_atlas.load()) {
      return -1;
    }

    const Camera3D camera{
      .position = player_initial_position + Vector3(0, PlayerController::eye_height, 0),
      .up = {0.0f, 1.0f, 0.0f},
      .fovy = 75.0f,
      .projection = CAMERA_PERSPECTIVE
    };

    PlayerController controller(
      world,
      camera,
      player_initial_position,
      {0.6f, 1.8f, 0.6f}
    );
    WorldPhysics physics(world, controller);
    WorldMesh world_mesh(world, texture_atlas);


    reset(world, nullptr, player, controller, physics);
    Renderer renderer(world, world_mesh, player, controller);
    SetTargetFPS(60);
    bool show_physics_debug = false;

    auto timers = DebugTimers{};
    auto start = Timer::now();


    while (!WindowShouldClose()) {
      const PlayerInput input = PlayerInput::poll();

      if (input.wants_toggle_debug()) {
        show_physics_debug = !show_physics_debug;
      }
      if (input.wants_reset()) {
        reset(world, &world_mesh, player, controller, physics);
      }

      // The targeting ray sees the mouse movement of this frame.

      start = Timer::now();
      controller.apply_look(input.look_delta);
      timers.apply_look = Timer::elapsed_ms(start);
      player.find_target(world, controller.camera);
      timers.find_target = Timer::elapsed_ms(start);
      player.manipulate_blocks(world, physics, controller, input);
      timers.manipulate_blocks = Timer::elapsed_ms(start);

      // Constant-acceleration movement and AABB collisions at 60 Hz.
      controller.update(input.movement_command(), GetFrameTime(), physics);
      timers.controller_update = Timer::elapsed_ms(start);


      std::vector<Vector3Int> blocks_overlapping_player = controller.blocks_overlapping_with_player();

      // todo: off main thread
      world_mesh.rebuild_dirty_meshes();
      timers.mesh_rebuild = Timer::elapsed_ms(start);

      BeginDrawing();
      ClearBackground(RAYWHITE);
      BeginMode3D(controller.camera);

      renderer.draw_world();
      renderer.draw_player();
      // todo: this doesn't really represent render time
      timers.render = Timer::elapsed_ms(start);

      EndMode3D();
      renderer.draw_ui(show_physics_debug, timers, blocks_overlapping_player);
      EndDrawing();

    }

    world_mesh.unload();
    texture_atlas.unload();

    CloseWindow();
    return 0;
  }
private:
  Vector3 player_initial_position{
    World::width / 2.0f,
    1.0f,
    World::width / 2.0f
  };


  void reset(
    World& world,
    WorldMesh* world_mesh,
    Player& player,
    PlayerController& controller,
    const WorldPhysics& physics
  ) {
    world.initialise();
    controller.reset(player_initial_position, physics);
    player.target.reset();
    if (world_mesh)
      world_mesh->rebuild_dirty_meshes();

  }
};