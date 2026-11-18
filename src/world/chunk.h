#pragma once

#include <vector>

#include "raylib.h"

#include "chunk_section.h"

struct World;

struct Chunk {

  World &world;
  Vector3 origin;
  BoundingBox bounds;


  Chunk(World& world, Vector3 origin) : world(world), origin(origin) {
    chunk_sections.reserve(n_sections);
    for (int i = 0; i < n_sections; i++) {
      chunk_sections.emplace_back(
        world, origin + Vector3(0, ChunkSection::n * i, 0)
      );
    }
    bounds = {
      origin,
      origin + Vector3(ChunkSection::n, n_sections*ChunkSection::n, ChunkSection::n)
    };
  }

  // todo: explicitly disallow copying and moving
  // Chunk(const Chunk &chunk) = delete;
  // Chunk(Chunk &&chunk) = delete;
  // Chunk& operator=(const Chunk &chunk) = delete;
  // Chunk& operator=(Chunk &&chunk) = delete;

  // returns the indexes of the sections that are now dirty
  std::vector<int> fill(BlockType block_type) {
    std::vector<int> dirty{};
    for (int i = 0; i < chunk_sections.size(); i++) {
      ChunkSection & chunk_section = chunk_sections[i];
      if (chunk_section.fill(block_type)) {
        dirty.push_back(i);
      }
    }
    return dirty;
  }

  static constexpr int n_sections = 4;
  std::vector<ChunkSection> chunk_sections;

};