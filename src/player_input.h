#pragma once

#include <raylib.h>
#include "player/player_controller.h"
#include "player/block_action.h"

// A snapshot of all game controls for one rendered frame.
// struct PlayerInput poll();
struct PlayerInput {
  Vector2 movement{};   // x: forward/back, y: right/left
  Vector2 look_delta{};
  bool jump = false;
  bool place_block = false;
  bool remove_block = false;
  bool reset_world = false;
  bool toggle_debug = false;

  static PlayerInput poll() {
    PlayerInput input;
    input.movement.x = static_cast<float>(
      (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) -
      (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
    );
    input.movement.y = static_cast<float>(
      (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) -
      (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
    );
    input.look_delta = GetMouseDelta();
    input.jump = IsKeyPressed(KEY_SPACE);
    input.place_block = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    input.remove_block = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    input.reset_world = IsKeyPressed(KEY_R);
    input.toggle_debug = IsKeyPressed(KEY_F3);
    return input;
  }


  BlockAction block_action() const noexcept {
    if (remove_block) return BlockAction::Remove;
    if (place_block) return BlockAction::Place;
    return BlockAction::None;
  }
  bool wants_reset() const { return reset_world; }
  bool wants_toggle_debug() const { return toggle_debug; }

  PlayerMovementInput movement_command() const {
    return {.movement = movement};
  }
};
