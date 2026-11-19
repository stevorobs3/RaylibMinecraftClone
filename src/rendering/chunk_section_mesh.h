#pragma once
#include <vector>
#include <utility>
#include <raylib.h>
#include "mesh_generator.h"

// Exclusive owner of the Model (and its meshes); not its shared atlas texture.
class OwnedModel {
public:
  explicit OwnedModel(Model model) noexcept : model_(model) {}
  ~OwnedModel() noexcept { UnloadModel(model_); }
  OwnedModel(const OwnedModel&)=delete;
  OwnedModel& operator=(const OwnedModel&)=delete;
  OwnedModel(OwnedModel&& other) noexcept : model_(std::exchange(other.model_,Model{})) {}
  OwnedModel& operator=(OwnedModel&& other) noexcept {
    if(this!=&other) {
      UnloadModel(model_);
      model_=std::exchange(other.model_,Model{});
    }
    return *this;
  }
  const Model& get() const noexcept { return model_; }
private:
  Model model_{};
};
class ChunkSectionMesh {
public:
  // All methods in this object access GPU resources: main thread only.
  bool replace(const std::vector<MeshData>& data,Texture2D atlas_texture);
  const std::vector<OwnedModel>& models() const noexcept { return models_; }
  void unload_gpu() noexcept { models_.clear(); }
private:
  std::vector<OwnedModel> models_;
};
