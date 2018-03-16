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
add_sources(pathfinder/src/renderer.cpp)
add_sources(pathfinder/src/pathfinder.cpp)
add_executable(pathfinder_demo ${SRCS})

find_package(OpenGL REQUIRED)
include_directories( ${OPENGL_INCLUDE_DIRS} )

find_package(GLEW REQUIRED)
if (GLEW_FOUND)
    include_directories(${GLEW_INCLUDE_DIRS})
    link_libraries(${GLEW_LIBRARIES})
endif()


add_subdirectory(3rdparty/glfw)
include_directories(3rdparty/glfw/include)
target_link_libraries(pathfinder_demo glfw ${GLFW_LIBRARIES})


target_link_libraries(pathfinder_demo ${OPENGL_LIBRARIES})
