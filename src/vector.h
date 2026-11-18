#pragma once

#include <string>
#include <format>
#include "raylib.h"

struct Vector3Int {
	int x,y,z;

	Vector3Int operator+(Vector3Int other) const {
		return Vector3Int(x + other.x, y + other.y, z + other.z);
	}
  Vector3Int operator+(Vector3 other) const {
    return Vector3Int(x + other.x, y + other.y, z + other.z);
  }

  bool operator==(Vector3Int other) const {
  	return other.x == x && other.y == y && other.z == z;
  }

  //todo: use this in print statements
  std::string to_string() const {
    return std::format("{},{},{}",x,y,z);
  }
};

inline std::string to_string(Vector3 vector3) {
  return std::format("{},{},{}",vector3.x,vector3.y,vector3.z);
}
