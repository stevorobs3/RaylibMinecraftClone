#pragma once

#include <array>
#include <utility>

#include "raylib.h"
#include "world/block.h"

enum class Direction {
  EAST,  // positive in x
  WEST,
  UP,    // positive in y
  DOWN,
  NORTH, // positive in z
  SOUTH
};

struct Quad {
  static constexpr int n_vertices = 4;

  using Vertices = std::array<Vector3, n_vertices>;

  BlockType block_type;
  Direction direction;
  Vector3Int position;
  Vertices vertices{};
  Vector3 normal;
  Color color;

  Quad(
    BlockType block_type,
    Direction direction,
    Vector3Int position
  ) : block_type(block_type), direction(direction), position(position){
    auto raw_verts = raw_vertices(direction);
    for (int i = 0; i < raw_verts.size(); i++) {
      vertices[i].x = position.x + raw_verts[i].x;
      vertices[i].y = position.y + raw_verts[i].y;
      vertices[i].z = position.z + raw_verts[i].z;
    }
    normal = lookup_normal(direction);

    if (direction == Direction::UP && block_type == BlockType::GRASS) {
      // todo: not right colour, look this up in grass block
      color = Color(145, 189, 89,255);
    }
    else {
      color =  WHITE;
    }

  }


  static constexpr std::array directions{
    Direction::EAST,
    Direction::WEST,
    Direction::UP,
    Direction::DOWN,
    Direction::NORTH,
    Direction::SOUTH
  };



  static constexpr Vector3 lookup_normal(Direction dir){
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


private:
    // anti-clockwise Right hand wind
  static constexpr Vertices north_face{
    Vector3{0,1,1},
    Vector3{1,1,1},
    Vector3{0,0,1},
    Vector3{1,0,1},
  };

  static constexpr Vertices south_face{
    Vector3{1,1,0},
    Vector3{0,1,0},
    Vector3{1,0,0},
    Vector3{0,0,0},
  };

  static constexpr Vertices east_face{
    Vector3{1,1,1},
    Vector3{1,1,0},
    Vector3{1,0,1},
    Vector3{1,0,0},
  };

  static constexpr Vertices west_face{
    Vector3{0,1,0},
    Vector3{0,1,1},
    Vector3{0,0,0},
    Vector3{0,0,1},
  };

  static constexpr Vertices top_face{
    Vector3{1,1,0},
    Vector3{1,1,1},
    Vector3{0,1,0},
    Vector3{0,1,1},
  };

  static constexpr Vertices bottom_face{
    Vector3{1,0,1},
    Vector3{1,0,0},
    Vector3{0,0,1},
    Vector3{0,0,0},
  };

  constexpr Vertices raw_vertices(Direction direction){
    switch (direction) {
      case Direction::EAST:  return east_face;
      case Direction::WEST:  return west_face;
      case Direction::UP:    return top_face;
      case Direction::DOWN:  return bottom_face;
      case Direction::NORTH: return north_face;
      case Direction::SOUTH: return south_face;
    }
    std::unreachable();
  }

};