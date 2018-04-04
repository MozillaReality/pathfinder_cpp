cmake_minimum_required (VERSION 2.6)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT WIN32)
  set(CMAKE_CXX_COMPILER "clang++")
endif()

project (pathfinder)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(pathfinder/include)

set(PATHFINDER_SRCS
  pathfinder/src/aa-strategy.cpp
  pathfinder/src/xcaa-strategy.cpp
  pathfinder/src/ssaa-strategy.cpp
  pathfinder/src/gl-utils.cpp
  pathfinder/src/renderer.cpp
  pathfinder/src/render-context.cpp
  pathfinder/src/buffer-texture.cpp
  pathfinder/src/meshes.cpp
  pathfinder/src/shader-loader.cpp
  pathfinder/src/text.cpp
  pathfinder/src/text-renderer.cpp
  pathfinder/src/atlas.cpp
)

add_executable(pathfinder_text_demo text-demo.cpp ${PATHFINDER_SRCS})

# ---- OpenGL ----
find_package(OpenGL REQUIRED)
include_directories( ${OPENGL_INCLUDE_DIRS} )
target_link_libraries(pathfinder_text_demo ${OPENGL_LIBRARIES})

# ---- GLAD ----
add_subdirectory(3rdparty/glad)
include_directories(${GLAD_INCLUDE_DIRS})
target_link_libraries(pathfinder_text_demo glad)

# ---- GLFW ----
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE INTERNAL "Generate installation target")
add_subdirectory(3rdparty/glfw)
include_directories(3rdparty/glfw/include)
target_link_libraries(pathfinder_text_demo glfw ${GLFW_LIBRARIES})

# ---- Freetype 2 ----
add_subdirectory(3rdparty/freetype2)
include_directories(3rdparty/freetype2/include)
target_link_libraries(pathfinder_text_demo freetype)

# ---- Kraken-Math ----
add_subdirectory(3rdparty/kraken-math)
include_directories(3rdparty/kraken-math/include)
target_link_libraries(pathfinder_text_demo krakenmath)

