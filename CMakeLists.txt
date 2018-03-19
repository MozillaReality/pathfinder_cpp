cmake_minimum_required (VERSION 2.6)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT WIN32)
  set(CMAKE_CXX_COMPILER "clang++")
endif()

macro (add_sources)
    file (RELATIVE_PATH _relPath "${PROJECT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")
    foreach (_src ${ARGN})
        if (_relPath)
            list (APPEND SRCS "${_relPath}/${_src}")
        else()
            list (APPEND SRCS "${_src}")
        endif()
    endforeach()
    if (_relPath)
        # propagate SRCS to parent directory
        set (SRCS ${SRCS} PARENT_SCOPE)
    endif()
endmacro()

project (pathfinder)


include_directories(pathfinder/include)
add_sources(main.cpp)
add_sources(pathfinder/src/gl-utils.cpp)
add_sources(pathfinder/src/renderer.cpp)
add_sources(pathfinder/src/pathfinder.cpp)
add_executable(pathfinder_demo ${SRCS})

find_package(OpenGL REQUIRED)
include_directories( ${OPENGL_INCLUDE_DIRS} )
target_link_libraries(pathfinder_demo ${OPENGL_LIBRARIES})

find_package(GLEW REQUIRED)
if (GLEW_FOUND)
    include_directories(${GLEW_INCLUDE_DIRS})
    link_libraries(${GLEW_LIBRARIES})
endif()


# GLFW
add_subdirectory(3rdparty/glfw)
include_directories(3rdparty/glfw/include)
target_link_libraries(pathfinder_demo glfw ${GLFW_LIBRARIES})

# Freetype 2
add_subdirectory(3rdparty/freetype2)
include_directories(3rdparty/freetype2/include/freetype)
target_link_libraries(pathfinder_demo freetype ${CMAKE_BINARY_DIR}/3rdparty/freetype2/libfreetype.a)

# Kraken-Math
add_subdirectory(3rdparty/kraken-math)
include_directories(3rdparty/kraken-math/include)
target_link_libraries(pathfinder_demo krakenmath ${CMAKE_BINARY_DIR}/3rdparty/kraken-math/libkrakenmath.a)

# include_directories(3rdparty/kraken-math/include)
# add_sources(3rdparty/kraken-math/src/aabb.cpp)
# add_sources(3rdparty/kraken-math/src/hitinfo.cpp)
# add_sources(3rdparty/kraken-math/src/krhelpers.cpp)
# add_sources(3rdparty/kraken-math/src/matrix4.cpp)
# add_sources(3rdparty/kraken-math/src/quaternion.cpp)
# add_sources(3rdparty/kraken-math/src/scalar.cpp)
# add_sources(3rdparty/kraken-math/src/triangle3.cpp)
# add_sources(3rdparty/kraken-math/src/vector2.cpp)
# add_sources(3rdparty/kraken-math/src/vector3.cpp)
# add_sources(3rdparty/kraken-math/src/vector4.cpp)


