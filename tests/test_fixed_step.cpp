#include "doctest.h"
#include "fixed_step.h"

TEST_CASE(
  "Fixed-step clock converts elapsed frame time into deterministic ticks"
) {
  FixedStepper clock;
  int ticks   = 0;
  float total = 0;
  clock.advance(FixedStepper::dt/2, [&](float dt){ticks++; total += dt; });
  CHECK(ticks == 0);
  clock.advance(FixedStepper::dt/2, [&](float dt){ticks++; total += dt; });
  CHECK(ticks == 1);
  CHECK(total == doctest::Approx(FixedStepper::dt));
  clock.reset();
  CHECK(clock.accumulator == doctest::Approx(0));
}

TEST_CASE("Fixed-step clock caps catch-up after a large stall") {
  FixedStepper clock;
  int ticks = 0;
  clock.advance(30.0f, [&](float){ticks++; });
  CHECK(ticks <= 6);
  CHECK(ticks >= 5);
}
