#pragma once

#include <raylib.h>

// A cutout chooses exactly one colour; alpha is not a blend weight.
// Half-opaque samples and above count as overlay, even at mip edges.
namespace BlockOverlay {
  inline constexpr float alpha_cutoff = 0.5f;

  constexpr bool visible(float alpha) noexcept {
    return alpha >= alpha_cutoff;
  }
}

// Owns one GPU program, shared but not owned by voxel Models.
// The Game owns this shader longer than WorldMesh.
class BlockShader {
public:
  BlockShader() noexcept = default;
  ~BlockShader() noexcept { unload(); }

  BlockShader(const BlockShader&) = delete;
  BlockShader& operator=(const BlockShader&) = delete;
  BlockShader(BlockShader&&) = delete;
  BlockShader& operator=(BlockShader&&) = delete;

  bool load();
  void unload() noexcept;

  Shader get() const noexcept { return shader_; }

private:
  Shader shader_{};
};
