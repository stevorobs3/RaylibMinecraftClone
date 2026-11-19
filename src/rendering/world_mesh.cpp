#include "world_mesh.h"

std::optional<MeshBuildResult> WorldMesh::prepare_cpu(SectionCoord coord) const {
  const ChunkSection* section=world_.find_section(coord);
  if(!section) return std::nullopt;
  auto snapshot=make_snapshot(world_,*section);
  const Chunk* chunk=world_.find_chunk(coord.chunk);
  return MeshBuildResult{coord,section->revision,chunk->generation,generator_.build(snapshot)};
}
bool WorldMesh::is_current(const MeshBuildResult& result) const noexcept {
  const ChunkSection* section=world_.find_section(result.section);
  const Chunk* chunk=world_.find_chunk(result.section.chunk);
  return section && chunk && section->revision==result.revision &&
         chunk->generation==result.generation;
}
bool WorldMesh::submit_gpu(MeshBuildResult&& result) {
  if(!is_current(result)) return false;
  ChunkSection* section=world_.find_section(result.section);
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
  for(auto& [coord,chunk]:world_.chunks())
    for(auto& section:chunk.chunk_sections) {
      if(!section.dirty) continue;
      auto result=prepare_cpu(section.coord);
      if(result) submit_gpu(std::move(*result));
    }
}
void WorldMesh::rebuild_all_meshes() {
  world_.mark_all_dirty();
  rebuild_dirty_meshes();
}
