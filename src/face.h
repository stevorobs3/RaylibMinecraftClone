#pragma once

#include <array>
#include <utility>

#include "raylib.h"


namespace Face {

  enum class Direction {
    EAST,  // positive in x
    WEST,
    UP,    // positive in y
    DOWN,
    NORTH, // positive in z
    SOUTH
  };

  static constexpr int n_vertices = 4;

  using FaceVertices = std::array<Vector3, n_vertices>;

  // anti-clockwise Right hand wind
  static constexpr FaceVertices north_face{
    Vector3{0,1,1},
    Vector3{1,1,1},
    Vector3{0,0,1},
    Vector3{1,0,1},
  };

  static constexpr FaceVertices south_face{
    Vector3{1,1,0},
    Vector3{0,1,0},
    Vector3{1,0,0},
    Vector3{0,0,0},
  };

  static constexpr FaceVertices east_face{
    Vector3{1,1,1},
    Vector3{1,1,0},
    Vector3{1,0,1},
    Vector3{1,0,0},
  };

  static constexpr FaceVertices west_face{
    Vector3{0,1,0},
    Vector3{0,1,1},
    Vector3{0,0,0},
    Vector3{0,0,1},
  };

  static constexpr FaceVertices top_face{
    Vector3{1,1,0},
    Vector3{1,1,1},
    Vector3{0,1,0},
    Vector3{0,1,1},
  };

  static constexpr FaceVertices bottom_face{
    Vector3{1,0,1},
    Vector3{1,0,0},
    Vector3{0,0,1},
    Vector3{0,0,0},
  };

  static constexpr std::array directions{
    Direction::EAST,
    Direction::WEST,
    Direction::UP,
    Direction::DOWN,
    Direction::NORTH,
    Direction::SOUTH
  };

  static constexpr FaceVertices vertices(Direction dir){
    switch (dir) {
      case Direction::EAST:  return east_face;
      case Direction::WEST:  return west_face;
      case Direction::UP:    return top_face;
      case Direction::DOWN:  return bottom_face;
      case Direction::NORTH: return north_face;
      case Direction::SOUTH: return south_face;
    }
    std::unreachable();
  }

  static constexpr Direction opposite(Direction dir){
    switch (dir) {
      case Direction::EAST: return Direction::WEST;
      case Direction::WEST: return Direction::EAST;
      case Direction::UP: return Direction::DOWN;
      case Direction::DOWN: return Direction::UP;
      case Direction::NORTH: return Direction::SOUTH;
      case Direction::SOUTH: return Direction::NORTH;
    }
    std::unreachable();
  }

  static constexpr Vector3 normal(Direction dir){
    switch (dir) {
      case Direction::EAST:  return { 1, 0,  0};
      case Direction::WEST:  return {-1, 0,  0};
      case Direction::UP:    return {0,  1,  0};
      case Direction::DOWN:  return {0, -1,  0};
      case Direction::NORTH: return {0,  0,  1};
      case Direction::SOUTH: return {0,  0, -1};
    }
    std::unreachable();
  }


};