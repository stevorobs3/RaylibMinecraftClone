#include "doctest.h"
#include "world/world.h"
#include "world_physics.h"
#include <cmath>

TEST_CASE(
  "Floor division and local coordinates reconstruct signed world positions"
) {
  for (int x = -257; x <= 257; x++) {
    const int q = floor_div(x, 16), r = floor_mod(x, 16);
    CHECK(r >= 0);
    CHECK(r<16);
    CHECK(16*q+r == x);
    CHECK(World::to_chunk_coord({x, 0, 0}).x == q);
    CHECK(World::to_local_position({x, 0, 0}).x == r);
  }
}

TEST_CASE("Swept AABB halts varying displacements before an obstacle") {
  World world;
  world.set(0, 7, 0, BlockType::STONE);
  WorldPhysics physics(world);
  for (int step = 0; step < 100; step++) {
    const float start  = -1.0f-static_cast<float>(step%13)*0.14f;
    const float travel = 3.0f+static_cast<float>(step%7);
    Vector3 feet{start, 7, 0.5f};
    Vector3 velocity{travel, 0, 0};
    physics.move_player(feet, velocity, {0.6f, 1.8f, 0.6f}, {travel, 0,
      0});
    CHECK(feet.x <= -0.3f+1e-5f);
    CHECK(velocity.x == doctest::Approx(0));
  }
}
