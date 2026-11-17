#pragma once

#include <cstdio>

// Accumulate measurements without printing inside timed graphics calls.
// This is main-thread-only profiling (not thread-safe).
namespace MeshProfiling {
  struct Counter {
    double total_ms = 0.0;
    double max_ms = 0.0;
    unsigned count = 0;

    void add(double ms) {
      total_ms += ms;
      if (ms > max_ms) max_ms = ms;
      ++count;
    }
    void print(const char* name) const {
      if (!count) return;
      std::printf("%-24s %6u calls  avg %8.3f ms  max %8.3f ms  total %9.3f ms\n",
                  name, count, total_ms / count, max_ms, total_ms);
    }
  };

  inline Counter cpu_build;
  inline Counter gpu_upload;
  inline Counter mesh_unload;

  inline void print_summary() {
    std::puts("\n=== Mesh profiling (rebuilds only) ===");
    cpu_build.print("CPU mesh build");
    gpu_upload.print("UploadMesh");
    mesh_unload.print("UnloadMesh");
    std::fflush(stdout);
  }
}
