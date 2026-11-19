#include "doctest.h"
#include "rendering/chunk_snapshot.h"
#include "rendering/world_mesh.h"

TEST_CASE(
  "Snapshot includes neighbours across chunk and section boundaries"
) {
  World world;
  world.set(-1, 30, 0, BlockType::STONE);
  world.set(0, 30, 0, BlockType::DIRT);
  auto* section = world.find_section({{-1, 0}, 1});
  REQUIRE(section != nullptr);
  const auto snapshot = make_snapshot(world, *section);
  CHECK(snapshot.at(15, 14, 0) == BlockType::STONE);
  CHECK(snapshot.at(16, 14, 0) == BlockType::DIRT);
  CHECK(snapshot.at(-1, 14, 0) == BlockType::AIR); // chunk -2 is unloaded
}

TEST_CASE("CPU work results are invalidated after edits") {
  World world;
  TextureAtlas atlas;
  BlockShader shader;
  WorldMesh meshes(world, atlas, shader);
  const SectionCoord coord{{0, 0}, 2};
  auto built = meshes.prepare_cpu(coord);
  REQUIRE(built.has_value());
  CHECK(meshes.is_current(*built));
  world.set(1, 33, 1, BlockType::STONE);
  CHECK_FALSE(meshes.is_current(*built));
  auto updated = meshes.prepare_cpu(coord);
  REQUIRE(updated.has_value());
  CHECK(meshes.is_current(*updated));
}

TEST_CASE(
  "Chunk generation disambiguates reloads even if the revision repeats"
) {
  World world;
  TextureAtlas atlas;
  BlockShader shader;
  WorldMesh meshes(world, atlas, shader);
  const SectionCoord coord{{0, 0}, 3};
  auto old = meshes.prepare_cpu(coord);
  REQUIRE(old.has_value());
  world.initialise();
  CHECK_FALSE(meshes.is_current(*old));
  auto fresh = meshes.prepare_cpu(coord);
  REQUIRE(fresh.has_value());
  CHECK(meshes.is_current(*fresh));
}

TEST_CASE("Captured mesh jobs remain immutable after world edits") {
  World world;
  TextureAtlas atlas;
  BlockShader shader;
  WorldMesh mesh(world, atlas, shader);
  const SectionCoord coord{{0, 0}, 3};
  auto request = mesh.capture_snapshot(coord);
  REQUIRE(request.has_value());
  CHECK(request->snapshot.at(5, 5, 5) == BlockType::AIR);
  world.set(5, 53, 5, BlockType::STONE);
  CHECK(request->snapshot.at(5, 5, 5) == BlockType::AIR);
  auto result = mesh.build_cpu(*request);
  CHECK(result.geometry.empty());
  CHECK_FALSE(mesh.is_current(result));
}
