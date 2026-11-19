cmake_minimum_required(VERSION 3.22)
project(RaylibMinecraft LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# Build the pinned raylib version with its bundled GLFW / GLAD.
set(PLATFORM "Desktop" CACHE STRING "" FORCE)
set(OPENGL_VERSION "3.3" CACHE STRING "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/6.2.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(raylib)

FetchContent_Declare(
    mdspan
    URL https://github.com/kokkos/mdspan/archive/refs/tags/mdspan-0.6.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(mdspan)

# All game implementations live in this library. Keep main.cpp out of it.
# Listing sources explicitly avoids silently missing files or compiling
# unintended sources when directories/extensions change.
add_library(minecraft_core STATIC
    src/player/player.cpp
    src/player/player_controller.cpp
    src/rendering/chunk_section_mesh.cpp
    src/rendering/mesh_generator.cpp
    src/rendering/render.cpp
    src/rendering/texture_atlas.cpp
    src/rendering/world_mesh.cpp
    src/world/world.cpp
    src/world_physics.cpp
    src/game.cpp
)

# Headers in src/ are shared by the game and tests.
target_include_directories(minecraft_core
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src"
    PRIVATE "${raylib_SOURCE_DIR}/src"
)

target_compile_definitions(minecraft_core PUBLIC
    RAYMATH_ENABLE_CPP_OPERATORS
)

target_link_libraries(minecraft_core PUBLIC
    raylib
    std::mdspan
)

# The application supplies only the entry point; everything else is in core.
add_executable(minecraft_clone src/main.cpp)
target_link_libraries(minecraft_clone PRIVATE minecraft_core)

# Some application headers use raylib 6.2's bundled GLAD declarations.
# This is an internal raylib header, so keep the version pinned above.
target_include_directories(minecraft_clone PRIVATE
    "${raylib_SOURCE_DIR}/src"
)

# Faithful textures are static development assets. Copy them once per build
# directory; the stamp prevents repeating the copy after every relink.
# Delete assets.stamp to force a refresh after changing source textures.
set(BLOCK_TEXTURE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/faithful/assets/minecraft/textures/block"
)
if(EXISTS "${BLOCK_TEXTURE_DIR}")
    set(ASSET_STAMP "${CMAKE_CURRENT_BINARY_DIR}/assets.stamp")
    add_custom_command(
        OUTPUT "${ASSET_STAMP}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${BLOCK_TEXTURE_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}/assets"
        COMMAND ${CMAKE_COMMAND} -E touch "${ASSET_STAMP}"
        COMMENT "Copying game assets (first build only)"
        VERBATIM
    )
    add_custom_target(copy_assets DEPENDS "${ASSET_STAMP}")
    add_dependencies(minecraft_clone copy_assets)
endif()

# Tests link the same core implementation as the application.
include(CTest)
if(BUILD_TESTING)
    add_executable(minecraft_tests
        tests/test_main.cpp
        tests/test_vector.cpp
        tests/test_coords.cpp
        tests/test_world.cpp
        tests/test_chunk_section.cpp
        tests/test_world_physics.cpp
        tests/test_player_actions.cpp
        tests/test_player_controller.cpp
        tests/test_quad.cpp
        tests/test_face_appearance.cpp
        tests/test_mesh_generator.cpp
        tests/test_chunk_snapshot.cpp
        tests/test_ownership.cpp
        tests/test_fixed_step.cpp
    )
    target_include_directories(minecraft_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party"
    )
    target_link_libraries(minecraft_tests PRIVATE minecraft_core)
    add_test(NAME unit_tests COMMAND minecraft_tests)
endif()
