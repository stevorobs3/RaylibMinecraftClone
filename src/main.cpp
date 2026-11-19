#include <exception>
#include <raylib.h>
#include "game.h"

namespace {
struct WindowLifetime {
  ~WindowLifetime() noexcept { CloseWindow(); }
};
}
int main() {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(1280, 720, "Raylib minecraft clone");
  WindowLifetime window{}; // outlives the Game and all GPU resources
  DisableCursor();
  PollInputEvents();
  SetTargetFPS(60);
  try {
    Game game;
    return game.run();
  } catch (const std::exception& error) {
    TraceLog(LOG_ERROR, "Game failed: %s", error.what());
    return 1;
  }
}
