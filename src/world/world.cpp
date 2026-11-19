#include "world.h"

World::World() { initialise(); }

void World::initialise() {
  chunks.clear();
  constexpr int start = -initial_grid_size/2;
  for (int z=0;z<initial_grid_size;++z)
    for (int x=0;x<initial_grid_size;++x)
      get_or_create_chunk({start+x,start+z});

  for (auto& [coord,chunk]:chunks) {
    for (int lx=0;lx<chunk_size;++lx)
      for (int lz=0;lz<chunk_size;++lz) {
        const int x=chunk.origin.x+lx;
        const int z=chunk.origin.z+lz;
        set(x,0,z,BlockType::BEDROCK);
        for (int y=1;y<=2;++y) set(x,y,z,BlockType::STONE);
        for (int y=3;y<=5;++y) set(x,y,z,BlockType::DIRT);
        set(x,6,z,BlockType::GRASS);
      }
  }
}

bool World::contains(Vector3Int pos) const noexcept {
  return pos.y>=0 && pos.y<height && is_loaded(to_chunk_coord(pos));
}
bool World::is_loaded(ChunkCoord coord) const noexcept {
  return chunks.find(coord)!=chunks.end();
}
Chunk* World::find_chunk(ChunkCoord coord) noexcept {
  auto it=chunks.find(coord);
  return it==chunks.end()?nullptr:&it->second;
}
const Chunk* World::find_chunk(ChunkCoord coord) const noexcept {
  auto it=chunks.find(coord);
  return it==chunks.end()?nullptr:&it->second;
}
Chunk& World::get_or_create_chunk(ChunkCoord coord) {
  auto [it,inserted]=chunks.try_emplace(coord,coord);
  if (inserted) {
    for (int section=0;section<Chunk::n_sections;++section) {
      mark_dirty({{coord.x-1,coord.z},section});
      mark_dirty({{coord.x+1,coord.z},section});
      mark_dirty({{coord.x,coord.z-1},section});
      mark_dirty({{coord.x,coord.z+1},section});
    }
  }
  return it->second;
}
ChunkSection* World::find_section(SectionCoord coord) noexcept {
  Chunk* chunk=find_chunk(coord.chunk);
  if (!chunk || coord.y<0 || coord.y>=Chunk::n_sections) return nullptr;
  return &chunk->chunk_sections[coord.y];
}
const ChunkSection* World::find_section(SectionCoord coord) const noexcept {
  const Chunk* chunk=find_chunk(coord.chunk);
  if (!chunk || coord.y<0 || coord.y>=Chunk::n_sections) return nullptr;
  return &chunk->chunk_sections[coord.y];
}
std::optional<BlockType> World::try_get(Vector3Int pos) const noexcept {
  if(pos.y<0 || pos.y>=height) return std::nullopt;
  const auto* section=find_section({to_chunk_coord(pos),pos.y/chunk_size});
  if(!section) return std::nullopt;
  return section->get_local(to_local_position(pos));
}
void World::mark_dirty(SectionCoord coord) noexcept {
  if (auto* section=find_section(coord)) section->mark_dirty();
}
SetBlockResult World::set(Vector3Int pos,BlockType type) noexcept {
  if(pos.y<0 || pos.y>=height) return SetBlockResult::OutOfBounds;
  const ChunkCoord chunk_coord=to_chunk_coord(pos);
  auto* section=find_section({chunk_coord,pos.y/chunk_size});
  if(!section) return SetBlockResult::Unloaded;
  const Vector3Int local=to_local_position(pos);
  if(!section->set_local(local,type)) return SetBlockResult::Unchanged;
  const int s=pos.y/chunk_size;
  if(local.x==0) mark_dirty({{chunk_coord.x-1,chunk_coord.z},s});
  if(local.x==chunk_size-1) mark_dirty({{chunk_coord.x+1,chunk_coord.z},s});
  if(local.z==0) mark_dirty({{chunk_coord.x,chunk_coord.z-1},s});
  if(local.z==chunk_size-1) mark_dirty({{chunk_coord.x,chunk_coord.z+1},s});
  if(local.y==0) mark_dirty({chunk_coord,s-1});
  if(local.y==chunk_size-1) mark_dirty({chunk_coord,s+1});
  return SetBlockResult::Changed;
}
std::vector<Vector3Int> World::positions_within_range(Vector3 origin,int range) const {
  const Vector3Int center=to_grid_position(origin);
  std::vector<Vector3Int> result;
  for(int dx=-range;dx<=range;++dx)
    for(int dy=-range;dy<=range;++dy)
      for(int dz=-range;dz<=range;++dz) {
        const Vector3Int p=center+Vector3Int{dx,dy,dz};
        if(contains(p)) result.push_back(p);
      }
  return result;
}
