#include "world_mesh.h"

std::optional<MeshBuildResult> WorldMesh::prepare_cpu(SectionCoord coord) const {
  const ChunkSection* section=world_.find_section(coord);
  if(!section) return std::nullopt;
  auto snapshot=make_snapshot(world_,*section);
  return MeshBuildResult{coord,section->revision,generator_.build(snapshot)};
}
bool WorldMesh::submit_gpu(MeshBuildResult&& result) {
  ChunkSection* section=world_.find_section(result.section);
  if(!section || section->revision!=result.revision) return false;
  if(result.geometry.empty()) {
    chunk_section_meshes.erase(result.section);
    section->acknowledge(result.revision);
    return true;
  }
  auto [it,inserted]=chunk_section_meshes.try_emplace(result.section);
  if(!it->second.replace(result.geometry,atlas_.texture())) {
    if(inserted) chunk_section_meshes.erase(it);
    return false;
  }
  section->acknowledge(result.revision);
  return true;
}
void WorldMesh::rebuild_dirty_meshes() {
  // Purge GPU models for chunks that were unloaded.
  for(auto it=chunk_section_meshes.begin();it!=chunk_section_meshes.end();) {
    if(!world_.find_section(it->first)) it=chunk_section_meshes.erase(it);
    else ++it;
  }
  for(auto& [coord,chunk]:world_.chunks)
    for(auto& section:chunk.chunk_sections) {
      if(!section.dirty) continue;
      auto result=prepare_cpu(section.coord);
      if(result) submit_gpu(std::move(*result));
    }
}
void WorldMesh::rebuild_all_meshes() {
  for(auto& [coord,chunk]:world_.chunks)
    for(auto& section:chunk.chunk_sections) section.mark_dirty();
  rebuild_dirty_meshes();
}
