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

# All game implementations live in this library. Keep main.cxx out of it.
# Listing sources explicitly avoids silently missing files or compiling
# unintended sources when directories/extensions change.
add_library(minecraft_core STATIC
    src/player/player.cpp
    src/player_controller.cpp
    src/render.cpp
    src/world_physics.cpp
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
add_executable(minecraft_clone src/main.cxx)
target_link_libraries(minecraft_clone PRIVATE minecraft_core)

# Some application headers use raylib 6.2's bundled GLAD declarations.
# This is an internal raylib header, so keep the version pinned above.
target_include_directories(minecraft_clone PRIVATE
    "${raylib_SOURCE_DIR}/src"
)

# Copy assets beside the executable, rather than during a library build.
set(BLOCK_TEXTURE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/faithful/assets/minecraft/textures/block"
)
if(EXISTS "${BLOCK_TEXTURE_DIR}")
    add_custom_command(TARGET minecraft_clone POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${BLOCK_TEXTURE_DIR}"
            "$<TARGET_FILE_DIR:minecraft_clone>/assets"
        COMMENT "Copying game assets"
        VERBATIM
    )
endif()

# Tests link the same core implementation as the application.
include(CTest)
if(BUILD_TESTING)
    add_executable(minecraft_tests
        tests/test_main.cpp
        tests/test_vector.cpp
    )
    target_include_directories(minecraft_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party"
    )
    target_link_libraries(minecraft_tests PRIVATE minecraft_core)
    add_test(NAME unit_tests COMMAND minecraft_tests)
endif()
