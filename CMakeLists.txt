cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

set(MAZE_VERSION 1.0)

# increment this when we break backwards compatibility
set(MAZE_SOVERSION 1)

project(
        maze
        VERSION ${MAZE_VERSION}
        DESCRIPTION "A nice walk in a maze"
        HOMEPAGE_URL "https://gitub.com/tomconder/maze"
        LANGUAGES C CXX)

set(COPYRIGHT "Copyright 2025 Tom Conder")

message(STATUS "Getting the current git hash")
execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(GIT_SHA ${GIT_HASH} CACHE STRING "Hash this build was generated from")

message(STATUS "Configuring ${PROJECT_NAME} ${PROJECT_VERSION} ${GIT_SHA}")

if (NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD
            23
            CACHE STRING "")
endif ()

if (CMAKE_CXX_STANDARD LESS 23)
    message(
            WARNING
            "Incompatible C++ standard ${CMAKE_CXX_STANDARD}. This project requires C++23"
    )
endif ()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set a default build type if none was specified See
# https://blog.kitware.com/cmake-and-the-default-build-type/ for details.
set(default_build_type "Release")

option(ENABLE_PROFILING "Enables compilation with the Tracy profiler" OFF)
if (ENABLE_PROFILING)
    message(STATUS "Enabling Tracy")
    add_definitions(-DENABLE_PROFILING)

    option(TRACY_STATIC "" OFF)
    option(TRACY_ENABLE "" ON)
    option(TRACY_ON_DEMAND "" ON)
endif (ENABLE_PROFILING)

option(ENABLE_IMGUI "Enables compilation with the imgui user interface library" ON)
if (ENABLE_IMGUI)
    message(STATUS "Enabling imgui")
    add_definitions(-DENABLE_IMGUI)
endif (ENABLE_IMGUI)

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

# This function finds and replaces values in the current flag
function(replace_flag_value flag_var_name match_string replace_string)
    STRING(REPLACE "${match_string}" "${replace_string}" ${flag_var_name} "${${flag_var_name}}")
    STRING(STRIP "${${flag_var_name}}" ${${flag_var_name}})
    SET(${flag_var_name} "${${flag_var_name}}" PARENT_SCOPE)
endfunction()

# Use flag /Z7 to embed debug symbols into .obj files instead of creating an external .pdb. sccache needs this to
# cache effectively
if (MSVC)
    replace_flag_value(CMAKE_C_FLAGS_DEBUG "/Zi" "/Z7")
    replace_flag_value(CMAKE_CXX_FLAGS_DEBUG "/Zi" "/Z7")
    replace_flag_value(CMAKE_EXE_LINKER_FLAGS "/Zi" "/Z7")
endif ()

add_subdirectory(sponge)

add_subdirectory(game)
