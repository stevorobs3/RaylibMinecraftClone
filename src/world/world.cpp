
#include "world.h"
#include <stdexcept>

World::World() {
  TraceLog(LOG_INFO, "Creating World");
  chunks.reserve(n * n);
  for (int z = 0; z < n; z++) {
    for (int x = 0; x < n; x++) {
      chunks.emplace_back(*this, Vector3{
        static_cast<float>(x * ChunkSection::n),
        0,
        static_cast<float>(z * ChunkSection::n),
      });
    }
  }
  //todo: external initialisation / loading / unloading from disk
  initialise();
}

void World::initialise() {
  TraceLog(LOG_INFO, "Initialising World");
  for(int i = 0; i < chunks.size(); i++) {
    Chunk &chunk = chunks[i];

    // Reset player-edited blocks too.
    std::vector<int> dirty = chunk.fill(BlockType::AIR);
    for (int section : dirty) {
      mark_dirty(i, section);
    }

    auto blocks = chunk.chunk_sections[0].blocks();
    for (int x = 0; x < ChunkSection::n; x++) {
      for (int z = 0; z < ChunkSection::n; z++) {
        blocks[x,0,z] = BlockType::BEDROCK;
        blocks[x,1,z] = BlockType::STONE;
        blocks[x,2,z] = BlockType::STONE;

        blocks[x,3,z] = BlockType::DIRT;
        blocks[x,4,z] = BlockType::DIRT;
        blocks[x,5,z] = BlockType::DIRT;

        blocks[x,6,z] = BlockType::GRASS;
      }
    }
    mark_dirty(i, 0);
  }
}

void World::mark_dirty(int chunk_index, int chunk_section_index) {
  assert(chunk_index >= 0 && chunk_index < chunks.size());
  assert(chunk_section_index >= 0 &&  chunk_section_index < Chunk::n_sections);

  dirty_chunk_sections.emplace(chunk_index, chunk_section_index);
}

bool World::contains(Vector3Int position) const {
  return position.x >= 0 && position.x < width &&
    position.y >= 0 && position.y < height &&
    position.z >= 0 && position.z < width;
}

BlockType World::get(int x, int y, int z) const {
  if (!contains({x,y,z})) throw std::out_of_range("World::get: outside world");
  // todo: use global and local coords with a mdspan wrapper here
  int index = (x / ChunkSection::n) + n * (z / ChunkSection::n);

  int chunk_section_index = y / ChunkSection::n;
  int section_x = x % ChunkSection::n;
  int section_y = y % ChunkSection::n;
  int section_z = z % ChunkSection::n;
  BlockType block_type =  chunks[index]
    .chunk_sections[chunk_section_index]
    .const_blocks()[section_x, section_y, section_z];

  return block_type;
}

ChunkSection& World::get_chunk_section(
  int chunk_index,
  int chunk_section_index
) {
  assert(chunk_index >= 0 && chunk_index < chunks.size());
  assert(chunk_section_index >= 0 && chunk_section_index < Chunk::n_sections);
  return chunks[chunk_index].chunk_sections[chunk_section_index];
}

void World::set(int x, int y, int z, BlockType block_type) {
  if (!contains({x,y,z})) throw std::out_of_range("World::set: outside world");
  if (get(x,y,z) == block_type) return;
  int index = (x / ChunkSection::n) + n * (z / ChunkSection::n);
  assert(index >= 0 && index < chunks.size());
  int chunk_section_index = y / ChunkSection::n;
  assert(chunk_section_index >= 0 && chunk_section_index < Chunk::n_sections);
  int section_x = x % ChunkSection::n;
  int section_y = y % ChunkSection::n;
  int section_z = z % ChunkSection::n;
  chunks[index]
    .chunk_sections[chunk_section_index]
    .blocks()[section_x, section_y, section_z] = block_type;

  mark_dirty(index, chunk_section_index);

  // todo: pull into separate place - its handy
  // todo: this is messy as
  int chunk_x = (x / ChunkSection::n);
  int chunk_z = (z / ChunkSection::n);
  if (section_x == 0) {
    int neighbour_index = (chunk_x - 1) + n * chunk_z;
    if (chunk_x > 0)
      mark_dirty(neighbour_index, chunk_section_index);
  }
  if (section_x == ChunkSection::n - 1) {
    int neighbour_index = (chunk_x + 1) + n * chunk_z;
    if (chunk_x + 1 < n)
      mark_dirty(neighbour_index, chunk_section_index);
  }
  if (section_z == 0) {
    int neighbour_index = chunk_x + n * (chunk_z - 1);
    if (chunk_z > 0)
      mark_dirty(neighbour_index, chunk_section_index);
  }
  if (section_z == ChunkSection::n - 1) {
    int neighbour_index = chunk_x + n * (chunk_z + 1);
    if (chunk_z + 1 < n)
      mark_dirty(neighbour_index, chunk_section_index);
  }
  if (section_y == 0) {
    int neighbour_index = chunk_section_index - 1;
    if (neighbour_index >= 0)
      mark_dirty(index, neighbour_index);
  }
  if (section_y == ChunkSection::n - 1) {
    int neighbour_index = chunk_section_index + 1;
    if (neighbour_index < Chunk::n_sections)
      mark_dirty(index, neighbour_index);
  }

}

std::vector<Vector3Int> World::positions_within_range(
  Vector3 origin,
  int range
) const {

  Vector3Int origin_grid_position = to_grid_position(origin);

  std::vector<Vector3Int> positions{};
  for (int dx = -range; dx < range; dx++) {
    for (int dy = -range; dy < range; dy++) {
      for (int dz = -range; dz < range; dz++) {
        Vector3Int position = origin_grid_position + Vector3Int(dx, dy, dz);
        if (contains(position)) {
          positions.push_back(position);
        }
      }
    }
  }
  return positions;
}