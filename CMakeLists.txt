cmake_minimum_required(VERSION 3.24)

set(MAZE_VERSION 1.0)

# increment this when we break backwards compatibility
set(MAZE_SOVERSION 1)

project(
        Maze
        VERSION ${MAZE_VERSION}
        DESCRIPTION "A nice walk in a maze"
        HOMEPAGE_URL "https://gitub.com/tomconder/maze"
        LANGUAGES C CXX)

set(COPYRIGHT "Copyright 2023 Tom Conder")

set(GIT_SHA
        "no-hash"
        CACHE STRING "Hash this build was generated from")
string(
        SUBSTRING "${GIT_SHA}"
        0
        8
        GIT_SHORT_SHA)

message(STATUS "Configuring ${PROJECT_NAME} ${PROJECT_VERSION} ${GIT_SHA}")

if (NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD
          20
          CACHE STRING "")
endif ()

if (CMAKE_CXX_STANDARD LESS 20)
  message(
          WARNING
          "Incompatible C++ standard ${CMAKE_CXX_STANDARD}. This project requires C++20"
  )
endif ()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set a default build type if none was specified See
# https://blog.kitware.com/cmake-and-the-default-build-type/ for details.
set(default_build_type "Release")

if (EXISTS "${CMAKE_SOURCE_DIR}/.git")
  set(default_build_type "Debug")
endif ()

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(
          STATUS
          "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE
          "${default_build_type}"
          CACHE STRING "Choose the type of build." FORCE)

  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif ()

# Generate compile_commands.json to help work with clang based tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enhance error reporting and compiler messages
set(CMAKE_COLOR_DIAGNOSTICS ON)

add_subdirectory(sponge)

add_subdirectory(game)
