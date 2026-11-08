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

# Jolt Physics
set(OVERRIDE_CXX_FLAGS OFF CACHE BOOL "" FORCE)
set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
set(ENABLE_ALL_WARNINGS OFF CACHE BOOL "" FORCE)
set(ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
		JoltPhysics
		URL https://github.com/jrouwe/JoltPhysics/archive/refs/tags/v5.6.0.zip
		SOURCE_SUBDIR Build
		DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(JoltPhysics)

file(GLOB SOURCES CONFIGURE_DEPENDS
		"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable(minecraft_clone ${SOURCES})

target_compile_definitions(minecraft_clone PRIVATE
		RAYMATH_ENABLE_CPP_OPERATORS
)


target_link_libraries(minecraft_clone PRIVATE
		raylib
		std::mdspan
		Jolt
)
# Deliberately use raylib 6.2's bundled GLAD declarations. This is an
# internal raylib header; keep the raylib version pinned above.
target_include_directories(minecraft_clone PRIVATE "${raylib_SOURCE_DIR}/src")
