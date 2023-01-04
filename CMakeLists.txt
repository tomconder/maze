cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

project(maze VERSION 1.0)

message(STATUS "Configuring maze ${maze_VERSION}")
configure_file(cmake/version.h.in version.h @ONLY)

if (NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17 CACHE STRING "")
endif ()

if (CMAKE_CXX_STANDARD LESS 17)
  message(WARNING "Incompatible C++ standard ${CMAKE_CXX_STANDARD}. This project requires C++17")
endif ()

# set(CMAKE_CXX_FLAGS "-std=c++11")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set a default build type if none was specified
# See https://blog.kitware.com/cmake-and-the-default-build-type/ for details.
set(default_build_type "Release")
if (EXISTS "${CMAKE_SOURCE_DIR}/.git")
  set(default_build_type "Debug")
endif ()

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)

  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif ()

mark_as_advanced(
        MAZE_LIB_DIRECTORY
        MAZE_BIN_DIRECTORY
)

set(MAZE_LIB_DIRECTORY "lib${LIB_SUFFIX}" CACHE STRING "Install path for libraries, e.g. 'lib64' on some 64-bit Linux distros.")
set(MAZE_BIN_DIRECTORY "bin" CACHE STRING "Install path for binaries")

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH};${CMAKE_INSTALL_PREFIX}/${MAZE_LIB_DIRECTORY}")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

add_subdirectory(3rdparty)

add_subdirectory(sponge)

add_subdirectory(game)
