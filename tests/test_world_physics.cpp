#include "doctest.h"
#include "world_physics.h"

TEST_CASE("Landing on the grass surface") {
  World world;
  WorldPhysics physics(world);
  Vector3 feet{16.5f, 7.1f, 16.5f};
  Vector3 velocity{0, -2, 0};
  physics.move_player(feet, velocity, {0.6f, 1.8f, 0.6f}, {0, -0.2f, 0});
  CHECK(feet.y == doctest::Approx(7.0f));
  CHECK(velocity.y == doctest::Approx(0.0f));
  const BoundingBox standing{{16.2f, 7.0f, 16.2f}, {16.8f, 8.8f, 16.8f}};
  CHECK(physics.is_grounded(standing));
}

TEST_CASE("Swept collision cannot tunnel through walls") {
  World world;
  world.set(16, 7, 16, BlockType::STONE);
  WorldPhysics physics(world);
  SUBCASE("positive X") {
    Vector3 feet{14.5f, 7, 16.5f};
    Vector3 velocity{100, 0, 0};
    physics.move_player(feet, velocity, {0.6f, 1.8f, 0.6f}, {4, 0, 0});
    CHECK(feet.x == doctest::Approx(15.7f));
    CHECK(velocity.x == doctest::Approx(0));
  }
  SUBCASE("negative X") {
    Vector3 feet{18.5f, 7, 16.5f};
    Vector3 velocity{-100, 0, 0};
    physics.move_player(feet, velocity, {0.6f, 1.8f, 0.6f}, {-4, 0, 0});
    CHECK(feet.x == doctest::Approx(17.3f));
    CHECK(velocity.x == doctest::Approx(0));
  }
}

TEST_CASE("Free movement preserves velocity") {
  World world;
  WorldPhysics physics(world);
  Vector3 feet{16.5f, 7.0f, 16.5f};
  Vector3 velocity{2, 0, 0};
  physics.move_player(feet, velocity, {0.6f, 1.8f, 0.6f}, {0.1f, 0, 0});
  CHECK(feet.x == doctest::Approx(16.6f));
  CHECK(velocity.x == doctest::Approx(2));
}

TEST_CASE("Sliding against a wall preserves parallel motion") {
  World world;
  world.set(16, 7, 16, BlockType::STONE);
  WorldPhysics physics(world);
  Vector3 feet{15.5f, 7.0f, 16.5f};
  Vector3 velocity{3, 0, 3};
  physics.move_player(feet, velocity, {0.6f, 1.8f, 0.6f}, {1, 0, 0.2f});
  CHECK(feet.x == doctest::Approx(15.7f));
  CHECK(feet.z == doctest::Approx(16.7f));
  CHECK(velocity.x == doctest::Approx(0));
  CHECK(velocity.z == doctest::Approx(3));
}

TEST_CASE("Air does not overlap a player") {
  World world;
  WorldPhysics physics(world);
  CHECK_FALSE(physics.player_overlaps_block(
    {1, 8, 1}, BlockType::AIR, {{1,8,1},{2,9,2}}
  ));
}
