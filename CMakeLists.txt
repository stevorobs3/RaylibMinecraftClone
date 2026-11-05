cmake_minimum_required(VERSION 3.22)
project(RaylibMinecraft LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# Build a matching version of raylib (and its bundled GLFW / GLAD).
set(PLATFORM "Desktop" CACHE STRING "" FORCE)
set(OPENGL_VERSION "3.3" CACHE STRING "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
		raylib
		URL "C:/Users/razer/dev/raylib-6.2.zip"
)
FetchContent_MakeAvailable(raylib)

add_executable(minecraft_clone src/main.cpp)
target_link_libraries(minecraft_clone PRIVATE raylib)

# Deliberately use raylib 6.2's bundled GLAD declarations. This is an
# internal raylib header; keep the raylib version pinned above.
target_include_directories(minecraft_clone PRIVATE "${raylib_SOURCE_DIR}/src")
