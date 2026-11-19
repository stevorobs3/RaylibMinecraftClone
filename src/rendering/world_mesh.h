#pragma once
#include <optional>
#include <unordered_map>
#include <vector>
#include "world/world.h"
#include "chunk_section_mesh.h"
#include "chunk_snapshot.h"
#include "mesh_generator.h"
#include "texture_atlas.h"

struct MeshBuildResult {
  SectionCoord section{};
  std::uint64_t revision=0;
  std::uint64_t generation=0;
  std::vector<MeshData> geometry;
};
struct WorldMesh {
  explicit WorldMesh(World& world,const TextureAtlas& atlas) noexcept
    : world_(world),atlas_(atlas),generator_(atlas) {}
  WorldMesh(const WorldMesh&)=delete;
  WorldMesh& operator=(const WorldMesh&)=delete;

  // CPU-only, future worker-safe after snapshot creation.
  std::optional<MeshBuildResult> prepare_cpu(SectionCoord coord) const;
  // GPU-only: rejects stale/unloaded section results.
  bool submit_gpu(MeshBuildResult&& result);
  bool is_current(const MeshBuildResult& result) const noexcept;

  void rebuild_dirty_meshes();
  void rebuild_all_meshes();
  void unload() noexcept { chunk_section_meshes.clear(); }
  std::unordered_map<SectionCoord,ChunkSectionMesh,SectionCoordHash> chunk_section_meshes;

private:
  World& world_;
  const TextureAtlas& atlas_;
  MeshGenerator generator_;
};
