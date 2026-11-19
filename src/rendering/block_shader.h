#pragma once

#include <raylib.h>

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
