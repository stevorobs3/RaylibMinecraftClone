#include "world_mesh.h"

std::optional<MeshBuildRequest> WorldMesh::capture_snapshot(SectionCoord coord) const {
  const ChunkSection* section=world_.find_section(coord);
  if(!section) return std::nullopt;
  const Chunk* chunk=world_.find_chunk(coord.chunk);
  return MeshBuildRequest{coord,section->revision,chunk->generation,
                          make_snapshot(world_,*section)};
}
MeshBuildResult WorldMesh::build_cpu(const MeshBuildRequest& request) const {
  return MeshBuildResult{request.section,request.revision,request.generation,
                         generator_.build(request.snapshot)};
}
std::optional<MeshBuildResult> WorldMesh::prepare_cpu(SectionCoord coord) const {
  auto request=capture_snapshot(coord);
  return request ? std::optional<MeshBuildResult>{build_cpu(*request)} : std::nullopt;
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
  auto it=chunk_section_meshes.find(result.section);
  if(it==chunk_section_meshes.end()) {
    // Build first; a failing allocation cannot leave a half-created map entry.
    ChunkSectionMesh replacement;
    if(!replacement.replace(result.geometry,atlas_.texture(),result.generation)) return false;
    chunk_section_meshes.try_emplace(result.section,std::move(replacement));
  } else {
    if(it->second.generation()!=result.generation) it->second.unload_gpu();
    if(!it->second.replace(result.geometry,atlas_.texture(),result.generation)) return false;
  }
  section->acknowledge(result.revision);
  return true;
}
void WorldMesh::rebuild_dirty_meshes() {
  // Purge GPU models for chunks that were unloaded.
  for(auto it=chunk_section_meshes.begin();it!=chunk_section_meshes.end();) {
    const Chunk* live=world_.find_chunk(it->first.chunk);
    if(!live || live->generation!=it->second.generation())
      it=chunk_section_meshes.erase(it);
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
