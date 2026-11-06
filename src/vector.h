#pragma once

struct Vector3Int {
	int x,y,z;

	Vector3Int operator+(Vector3Int other) const {
		return Vector3Int(x + other.x, y + other.y, z + other.z);
	}
};
