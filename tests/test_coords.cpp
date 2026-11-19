#include "doctest.h"
#include "world/coords.h"
#include "world/world.h"
#include <type_traits>
#include <unordered_map>

TEST_CASE("Signed division maps negative global positions to the correct chunk") {
  CHECK(floor_div(-17,16)==-2);
  CHECK(floor_div(-16,16)==-1);
  CHECK(floor_div(-1,16)==-1);
  CHECK(floor_div(0,16)==0);
  CHECK(floor_div(15,16)==0);
  CHECK(floor_div(16,16)==1);
  CHECK(floor_mod(-17,16)==15);
  CHECK(floor_mod(-16,16)==0);
  CHECK(floor_mod(-1,16)==15);
  CHECK(floor_mod(16,16)==0);
}
TEST_CASE("Coordinate key hashing distinguishes signed chunks and sections") {
  std::unordered_map<ChunkCoord,int,ChunkCoordHash> chunks;
  chunks[{0,0}]=1; chunks[{-1,0}]=2; chunks[{0,-1}]=3;
  CHECK(chunks.size()==3);
  CHECK(chunks.at({-1,0})==2);
  std::unordered_map<SectionCoord,int,SectionCoordHash> sections;
  sections[{{-1,0},0}]=7;
  sections[{{-1,0},1}]=8;
  CHECK(sections.size()==2);
}
TEST_CASE("Floating point positions floor into signed block coordinates") {
  CHECK(World::to_grid_position({-0.1f,0.1f,-16.1f})==Vector3Int(-1,0,-17));
  CHECK(World::to_grid_position({0.0f,16.0f,15.99f})==Vector3Int(0,16,15));
}
