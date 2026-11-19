#pragma once
#include <algorithm>

// One deterministic simulation clock shared by systems requiring fixed ticks.
// Clamping protects against huge catch-up bursts after a window stall.
struct FixedStepper {
  static constexpr float dt=1.0f/60.0f;
  float accumulator=0;
  void reset() noexcept { accumulator=0; }
  template<typename StepFn>
  void advance(float frame_dt,StepFn&& step) {
    accumulator += std::clamp(frame_dt,0.0f,0.1f);
    while(accumulator>=dt) {
      step(dt);
      accumulator-=dt;
    }
  }
};
