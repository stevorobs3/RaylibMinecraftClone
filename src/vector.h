#pragma once

#include <string>
#include <format>

struct Vector3Int {
	int x, y, z;

	Vector3Int operator+(Vector3Int other) const noexcept {
		return Vector3Int(x + other.x, y + other.y, z + other.z);
	}

  bool operator == (Vector3Int other) const noexcept {
  	return other.x == x && other.y == y && other.z == z;
  }

  std::string to_string() const {
    return std::format("{},{},{}", x, y, z);
  }
};
