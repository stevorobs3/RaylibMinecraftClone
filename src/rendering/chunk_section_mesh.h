#pragma once

#include <cstdint>
#include <utility>
#include <vector>

#include <raylib.h>

#include "mesh_generator.h"

// Owns the Model and its meshes, not the shared atlas texture.
class OwnedModel {
public:
  explicit OwnedModel(Model model) noexcept : model_(model) {}

  ~OwnedModel() noexcept {
    UnloadModel(model_);
  }

  OwnedModel(const OwnedModel&) = delete;
  OwnedModel& operator=(const OwnedModel&) = delete;

  OwnedModel(OwnedModel&& other) noexcept
    : model_(std::exchange(other.model_, Model{})) {}

  OwnedModel& operator=(OwnedModel&& other) noexcept {
    if (this != &other) {
      UnloadModel(model_);
      model_ = std::exchange(other.model_, Model{});
    }
    return *this;
  }

  const Model& get() const noexcept { return model_; }

private:
  Model model_{};
};

// GPU operations must execute on the graphics thread.
class ChunkSectionMesh {
public:
  ChunkSectionMesh() = default;

  ChunkSectionMesh(const ChunkSectionMesh&) = delete;
  ChunkSectionMesh& operator=(const ChunkSectionMesh&) = delete;

  ChunkSectionMesh(ChunkSectionMesh&&) noexcept = default;
  ChunkSectionMesh& operator=(ChunkSectionMesh&&) noexcept = default;

  bool replace(
    const std::vector<MeshData>& data,
    Texture2D atlas_texture,
    std::uint64_t generation
  );

  std::uint64_t generation() const noexcept {
    return generation_;
  }

  const std::vector<OwnedModel>& models() const noexcept {
    return models_;
  }

  void unload_gpu() noexcept {
    models_.clear();
    generation_ = 0;
  }

private:
  std::vector<OwnedModel> models_;
  std::uint64_t generation_ = 0;
};
