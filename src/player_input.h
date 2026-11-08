#pragma once

#include <raylib.h>

// Translate raylib polling into one game input snapshot per frame.
struct PlayerInput {
  Vector2 movement{};   // Forward/back and strafe right/left
  Vector2 look_delta{};
  bool jump = false;
  bool place_block = false;
  bool remove_block = false;
  bool reset_world = false;

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
    return input;
  }

  bool wants_jump() const { return jump; }
  bool wants_place_block() const { return place_block; }
  bool wants_remove_block() const { return remove_block; }
  bool wants_reset() const { return reset_world; }

  PlayerMovementInput movement_command() const {
    return {
      .movement = movement,
      .look_delta = look_delta,
      .jump = jump
    };
  }
};
