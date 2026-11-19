#pragma once
#include <optional>
#include <unordered_map>
#include <vector>
#include "world/world.h"
#include "chunk_section_mesh.h"
#include "chunk_snapshot.h"
#include "mesh_generator.h"
#include "texture_atlas.h"

struct MeshBuildRequest {
  SectionCoord section{};
  std::uint64_t revision   = 0;
  std::uint64_t generation = 0;
  ChunkSnapshot snapshot;
};
struct MeshBuildResult {
  SectionCoord section{};
  std::uint64_t revision   = 0;
  std::uint64_t generation = 0;
  std::vector<MeshData> geometry;
};

struct WorldMesh {
  explicit WorldMesh(World& world, const TextureAtlas& atlas) noexcept
    : world_(world), atlas_(atlas), generator_(atlas) {}
  WorldMesh(const WorldMesh&) = delete;
  WorldMesh& operator=(const WorldMesh&) = delete;

  // Main-thread capture: reads the live world into a snapshot.
  std::optional<MeshBuildRequest> capture_snapshot(
    SectionCoord coord
  ) const;
  // Pure CPU work: safe for workers with immutable inputs.
  MeshBuildResult build_cpu(const MeshBuildRequest& request) const;
  // Synchronous convenience path, useful until Milestone 3.
  std::optional<MeshBuildResult> prepare_cpu(SectionCoord coord) const;
  // GPU-only: rejects stale/unloaded section results.
  bool submit_gpu(MeshBuildResult&& result);
  bool is_current(const MeshBuildResult& result) const noexcept;

  void rebuild_dirty_meshes();
  void rebuild_all_meshes();
  void unload() noexcept {
    chunk_section_meshes.clear();
  }
  using SectionMeshes =
    std::unordered_map<
      SectionCoord, ChunkSectionMesh, SectionCoordHash
    >;
  SectionMeshes chunk_section_meshes;

private:
  World& world_;
  const TextureAtlas& atlas_;
  MeshGenerator generator_;
};
