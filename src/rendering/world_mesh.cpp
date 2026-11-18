
#include "world_mesh.h"

WorldMesh::WorldMesh(
  World &world,
  TextureAtlas& texture_atlas
) : world(world), texture_atlas(texture_atlas) {
// todo: more generally - what happens if something like the push_back fails?
  // todo: more generally - what error handling am I missing?
  // todo: more generally - the mesh generation requires run-time allocations, can this be mitigated / minimised / correctly error handled?
  chunk_section_meshes.reserve(world.num_chunks * Chunk::n_sections);
  for (int i = 0; i < world.num_chunks; i++) {
    for (int j = 0; j < Chunk::n_sections; j++) {
      ChunkSection &chunk_section = world.get_chunk_section(i,j);
      chunk_section_meshes.push_back(ChunkSectionMesh{
        chunk_section,
        texture_atlas
      });
    }
  }
}

void WorldMesh::rebuild_dirty_meshes() {
  for (auto [i,j] : world.dirty_chunk_sections) {
    int index = i * Chunk::n_sections + j;
    assert(index >= 0 && index < chunk_section_meshes.size());
    chunk_section_meshes[index].rebuild_mesh();
  }
  world.dirty_chunk_sections.clear();
}

void WorldMesh::unload() {
  for (ChunkSectionMesh &c : chunk_section_meshes) {
    c.unload();
  }
}