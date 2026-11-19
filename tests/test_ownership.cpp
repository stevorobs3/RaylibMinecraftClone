#include "doctest.h"
#include <type_traits>
#include <utility>
#include <raylib.h>
#include "rendering/texture_atlas.h"
#include "rendering/chunk_section_mesh.h"
#include "world/chunk.h"

static_assert(!std::is_copy_constructible_v<OwnedImage>);
static_assert(std::is_nothrow_move_constructible_v<OwnedImage>);
static_assert(!std::is_copy_constructible_v<OwnedModel>);
static_assert(std::is_nothrow_move_constructible_v<OwnedModel>);
static_assert(!std::is_move_constructible_v<Chunk>);
static_assert(!std::is_move_constructible_v<ChunkSection>);
static_assert(!std::is_copy_constructible_v<TextureAtlas>);
static_assert(!std::is_copy_constructible_v<ChunkSectionMesh>);
static_assert(std::is_nothrow_move_constructible_v<ChunkSectionMesh>);

TEST_CASE("Moving an owned image leaves exactly one owner") {
  OwnedImage owner(GenImageColor(4,4,WHITE));
  REQUIRE(owner.valid());
  OwnedImage replacement(std::move(owner));
  CHECK_FALSE(owner.valid());
  CHECK(replacement.valid());
  replacement.reset();
  CHECK_FALSE(replacement.valid());
}
