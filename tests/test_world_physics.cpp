#include "doctest.h"

#include "world_physics.h"

TEST_CASE("Player stops at the grass surface when falling") {
    World world;
    WorldPhysics physics(world);
    Vector3 feet{16.5f, 7.1f, 16.5f};
    Vector3 velocity{0.0f, -2.0f, 0.0f};
    const Vector3 size{0.6f, 1.8f, 0.6f};

    physics.move_player(feet, velocity, size, {0.0f, -0.2f, 0.0f},
                        {{16, 6, 16}});

    CHECK(feet.y == doctest::Approx(7.0f));
    CHECK(velocity.y == doctest::Approx(0.0f));
}

TEST_CASE("Player stops against a wall when moving in either X direction") {
    World world;
    world.set(16, 7, 16, BlockType::STONE);
    WorldPhysics physics(world);
    const Vector3 size{0.6f, 1.8f, 0.6f};
    const std::vector<Vector3Int> nearby{{16, 7, 16}};

    SUBCASE("moving toward positive X") {
        Vector3 feet{15.5f, 7.0f, 16.5f};
        Vector3 velocity{2.0f, 0.0f, 0.0f};
        physics.move_player(feet, velocity, size, {0.5f, 0.0f, 0.0f}, nearby);
        CHECK(feet.x == doctest::Approx(15.7f));
        CHECK(velocity.x == doctest::Approx(0.0f));
    }

    SUBCASE("moving toward negative X") {
        Vector3 feet{17.5f, 7.0f, 16.5f};
        Vector3 velocity{-2.0f, 0.0f, 0.0f};
        physics.move_player(feet, velocity, size, {-0.5f, 0.0f, 0.0f}, nearby);
        CHECK(feet.x == doctest::Approx(17.3f));
        CHECK(velocity.x == doctest::Approx(0.0f));
    }
}

TEST_CASE("Player moves freely without an intersecting block") {
    World world;
    WorldPhysics physics(world);
    Vector3 feet{16.5f, 7.0f, 16.5f};
    Vector3 velocity{2.0f, 0.0f, 0.0f};
    physics.move_player(feet, velocity, {0.6f, 1.8f, 0.6f},
                        {0.1f, 0.0f, 0.0f}, {{16, 6, 16}});
    CHECK(feet.x == doctest::Approx(16.6f));
    CHECK(velocity.x == doctest::Approx(2.0f));
}
