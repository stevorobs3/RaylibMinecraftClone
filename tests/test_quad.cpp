#include "doctest.h"
#include "rendering/quad.h"
#include <cmath>

TEST_CASE("All six voxel faces have matching winding and outward normals")
  {
  for (Direction direction : Quad::directions) {
    Quad quad{BlockType::STONE, direction, {0, 0, 0}};
    const auto a = quad.vertices[0];
    const auto b = quad.vertices[2];
    const auto c = quad.vertices[1];
    Vector3 cross{
      (b.y-a.y)*(c.z-a.z) - (b.z-a.z)*(c.y-a.y),
      (b.z-a.z)*(c.x-a.x) - (b.x-a.x)*(c.z-a.z),
      (b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x)
    };
    const float dot = cross.x*quad.normal.x + cross.y*quad.normal.y +
      cross.z*quad.normal.z;
    CHECK(std::abs(dot) == doctest::Approx(1.0f));
  }
}
