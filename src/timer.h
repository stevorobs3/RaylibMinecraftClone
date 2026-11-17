#pragma once
#include <chrono>

namespace Timer {
  inline auto now() {
    return std::chrono::steady_clock::now();
  }

  double elapsed_ms(auto & start) {
    double duration =  std::chrono::duration<double, std::milli>(
          now() - start
    ).count();
    start = now();
    return duration;
  }
}