cmake_minimum_required (VERSION 2.6)

project (pathfinder_demo)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(pathfinder_text_demo text-demo.cpp)

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

# ---- Hydra ----
add_subdirectory(3rdparty/hydra)
include_directories(3rdparty/hydra/include)
target_link_libraries(pathfinder_text_demo hydra)

# ---- Pathfinder ----
add_subdirectory(pathfinder)
include_directories(pathfinder/include)
target_link_libraries(pathfinder_text_demo pathfinder)

