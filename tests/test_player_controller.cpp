#include "doctest.h"
#include "player/player_controller.h"
#include "world_physics.h"

TEST_CASE("Controller keeps camera aligned with feet") {
  World world;
  WorldPhysics physics(world);
  Camera3D camera{
    .position={0.5f,8.62f,0.5f},
    .target={1.5f,8.62f,0.5f},
    .up={0,1,0},.fovy=75,.projection=CAMERA_PERSPECTIVE
  };
  PlayerController controller(physics,camera,{0.5f,7,0.5f},{0.6f,1.8f,0.6f});
  CHECK(camera.position.y==doctest::Approx(controller.feet.y+PlayerController::eye_height));
  controller.apply_look({10,5});
  CHECK(camera.position.x==doctest::Approx(controller.feet.x));
}
TEST_CASE("Fixed-step movement works with negative world coordinates") {
  World world;
  WorldPhysics physics(world);
  Camera3D camera{
    .position={-1.5f,8.62f,0.5f},
    .target={-0.5f,8.62f,0.5f},
    .up={0,1,0},.fovy=75,.projection=CAMERA_PERSPECTIVE
  };
  PlayerController controller(physics,camera,{-1.5f,7,0.5f},{0.6f,1.8f,0.6f});
  PlayerMovementInput input{{1,0}};
  controller.fixed_update(input,1.0f/60.0f);
  CHECK(controller.feet.x>-1.5f);
  CHECK(controller.feet.y==doctest::Approx(7));
  CHECK(controller.grounded);
  controller.reset({-1.5f,7,0.5f});
  CHECK(controller.feet.x==doctest::Approx(-1.5f));
  CHECK(controller.velocity.x==doctest::Approx(0));
}
