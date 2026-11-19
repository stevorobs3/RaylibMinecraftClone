
Issues / tasks:
  Move mesh building off the main thread
  Clean up / remove the timing code
  Check if the to_string of vector is used everywhere
  Re-write collisions and movement to use the other sliding AABB mechanism
  make player block manipulation based off some action enum
  Pull out the fixed_update stuff into a central place that other code can use
  refactor mesh_generator to be more tidy
  Lookup proper colors for minecraft overlays
  Implement overlaps properly the way minecraft does - transparent quad over top
  make the chunk boundary debug visuals more grid like
  tidy up lookup_texture_uv
  Add proper error handling everywhere
  Add proper RAII principles everywhere
  Revisit where dirty mesh state should be handled and how it should be updated
  Frustrum culling
  Air pocket culling
  Chunk and chunkSection copying and move semantics
  Use a PackedArray for chunks
  Chunk loading / unloading
  Proper world generation
  Refactor world coordinates into local (within a chunk / chunk section) and global
  Handle negative world coordinates
