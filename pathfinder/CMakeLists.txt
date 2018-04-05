cmake_minimum_required (VERSION 2.6)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(pathfinder)

set(PUBLIC_HEADERS
  include/pathfinder.h
)

set(SRCS
  src/aa-strategy.cpp
  src/xcaa-strategy.cpp
  src/ssaa-strategy.cpp
  src/gl-utils.cpp
  src/renderer.cpp
  src/render-context.cpp
  src/buffer-texture.cpp
  src/meshes.cpp
  src/shader-loader.cpp
  src/text.cpp
  src/text-renderer.cpp
  src/atlas.cpp
  src/pathfinder.cpp
  src/pathfinder-impl.cpp
)

add_library(pathfinder STATIC ${SRCS} ${PUBLIC_HEADERS})
