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

file(GLOB SOURCES CONFIGURE_DEPENDS
		"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_library(minecraft_core STATIC ${SOURCES})

target_compile_definitions(minecraft_core PRIVATE
		RAYMATH_ENABLE_CPP_OPERATORS
)

target_link_libraries(minecraft_core PRIVATE
  raylib
  std::mdspan
)
# Deliberately use raylib 6.2's bundled GLAD declarations. This is an
# internal raylib header; keep the raylib version pinned above.
target_include_directories(minecraft_core PRIVATE
  "${raylib_SOURCE_DIR}/src")


add_custom_command(TARGET minecraft_core POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  "${CMAKE_SOURCE_DIR}/assets/faithful/assets/minecraft/textures/block"
  "$<TARGET_FILE_DIR:minecraft_clone>/assets"
  COMMENT "Copying game assets")

# Main application
add_executable(minecraft_clone
  src/main.cxx
)

target_link_libraries(minecraft_clone PRIVATE
  minecraft_core
  raylib
  mdspan
)

# Unit tests
enable_testing()

file(GLOB TEST_SOURCES CONFIGURE_DEPENDS
		"${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
)



add_executable(minecraft_tests ${TEST_SOURCES})

target_include_directories(minecraft_tests PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/third_party"
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(minecraft_tests PRIVATE
  minecraft_core
  raylib
  mdspan
)

add_test(NAME unit_tests COMMAND minecraft_tests)