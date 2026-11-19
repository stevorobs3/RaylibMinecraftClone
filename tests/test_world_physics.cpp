#include "doctest.h"
#include "world_physics.h"

TEST_CASE("Falling player lands on grass without penetrating") {
  World world;
  WorldPhysics physics(world);
  Vector3 feet{0.5f,7.1f,0.5f};
  Vector3 velocity{0,-2,0};
  physics.move_player(feet,velocity,{0.6f,1.8f,0.6f},{0,-0.2f,0});
  CHECK(feet.y==doctest::Approx(7));
  CHECK(velocity.y==doctest::Approx(0));
  CHECK(physics.is_grounded({{0.2f,7,0.2f},{0.8f,8.8f,0.8f}}));
}

TEST_CASE("High speed movement cannot tunnel through blocks in either X direction") {
  World world;
  world.set(0,7,0,BlockType::STONE);
  WorldPhysics physics(world);
  SUBCASE("positive X") {
    Vector3 feet{-2.5f,7,0.5f},velocity{100,0,0};
    physics.move_player(feet,velocity,{0.6f,1.8f,0.6f},{4,0,0});
    CHECK(feet.x==doctest::Approx(-0.3f));
    CHECK(velocity.x==doctest::Approx(0));
  }
  SUBCASE("negative X") {
    Vector3 feet{2.5f,7,0.5f},velocity{-100,0,0};
    physics.move_player(feet,velocity,{0.6f,1.8f,0.6f},{-4,0,0});
    CHECK(feet.x==doctest::Approx(1.3f));
    CHECK(velocity.x==doctest::Approx(0));
  }
}

TEST_CASE("Sliding against a wall preserves tangential movement") {
  World world;
  world.set(0,7,0,BlockType::STONE);
  WorldPhysics physics(world);
  Vector3 feet{-0.5f,7,0.5f},velocity{3,0,3};
  physics.move_player(feet,velocity,{0.6f,1.8f,0.6f},{1,0,0.2f});
  CHECK(feet.x==doctest::Approx(-0.3f));
  CHECK(feet.z==doctest::Approx(0.7f));
  CHECK(velocity.x==doctest::Approx(0));
  CHECK(velocity.z==doctest::Approx(3));
}

TEST_CASE("Collision also works with negative world coordinates") {
  World world;
  world.set(-2,7,-3,BlockType::STONE);
  WorldPhysics physics(world);
  Vector3 feet{-4.5f,7,-2.5f},velocity{30,0,0};
  physics.move_player(feet,velocity,{0.6f,1.8f,0.6f},{4,0,0});
  CHECK(feet.x==doctest::Approx(-2.3f));
  CHECK(velocity.x==doctest::Approx(0));
}

TEST_CASE("No obstacles leave movement and velocity unchanged") {
  World world;
  WorldPhysics physics(world);
  Vector3 feet{0.5f,7,0.5f},velocity{2,0,0};
  physics.move_player(feet,velocity,{0.6f,1.8f,0.6f},{0.1f,0,0});
  CHECK(feet.x==doctest::Approx(0.6f));
  CHECK(velocity.x==doctest::Approx(2));
}
TEST_CASE("AIR never collides") {
  World world;
  WorldPhysics physics(world);
  CHECK_FALSE(physics.player_overlaps_block({0,8,0},BlockType::AIR,
                                          {{0,8,0},{1,9,1}}));
}
