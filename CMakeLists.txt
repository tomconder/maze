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

set(COPYRIGHT "Copyright 2026 Tom Conder")

message(STATUS "Getting the current git hash")
execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE)

set(GIT_SHA
        ${GIT_HASH}
        CACHE STRING "Hash this build was generated from")

message(STATUS "Configuring ${PROJECT_NAME} ${PROJECT_VERSION} ${GIT_SHA}")

if (NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 23)
else ()
    if (CMAKE_CXX_STANDARD LESS 23)
        message(
                WARNING
                "Incompatible C++ standard ${CMAKE_CXX_STANDARD}. This project requires C++23"
        )
    endif ()
endif ()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

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

option(ENABLE_IMGUI "Enables compilation with the imgui user interface library"
        OFF)
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

# Enhance error reporting and compiler messages
set(CMAKE_COLOR_DIAGNOSTICS ON)

if (MSVC)
    # This enables Edit & Continue support. See
    # https://learn.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio#edit-and-continue-for-cmake-projects
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>")
    add_link_options("$<$<CONFIG:Debug>:/INCREMENTAL>")
endif ()

add_subdirectory(sponge)

# --- Shader-Slang compilation ---

find_program(SLANGC_EXECUTABLE slangc
    REQUIRED
    HINTS
        "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows/tools/shader-slang"
        "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-linux/tools/shader-slang"
        "${CMAKE_SOURCE_DIR}/vcpkg_installed/arm64-osx/tools/shader-slang"
)
message(STATUS "Found slangc: ${SLANGC_EXECUTABLE}")

set(SLANG_SOURCE_DIR "${CMAKE_SOURCE_DIR}/assets/shaders/slang")
set(SLANG_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders/glsl")
file(MAKE_DIRECTORY "${SLANG_OUTPUT_DIR}")

# Usage: compile_slang_shader(<slang-file> <entry-point> <output-glsl-name>)
# Appends the output path to SLANG_COMPILED_OUTPUTS (parent scope).
function(compile_slang_shader SLANG_FILE ENTRY_POINT OUTPUT_NAME)
    set(INPUT  "${SLANG_SOURCE_DIR}/${SLANG_FILE}")
    set(OUTPUT "${SLANG_OUTPUT_DIR}/${OUTPUT_NAME}")
    file(GLOB SLANG_INCLUDES "${SLANG_SOURCE_DIR}/include/*.slang")
    add_custom_command(
        OUTPUT  "${OUTPUT}"
        COMMAND "${SLANGC_EXECUTABLE}"
                "${INPUT}"
                -entry "${ENTRY_POINT}"
                -target glsl
                -profile glsl_450
                -matrix-layout-column-major
                -o "${OUTPUT}"
        DEPENDS "${INPUT}" ${SLANG_INCLUDES}
        COMMENT "slangc: ${SLANG_FILE} [${ENTRY_POINT}] -> ${OUTPUT_NAME}"
        VERBATIM
    )
    set(SLANG_COMPILED_OUTPUTS "${SLANG_COMPILED_OUTPUTS};${OUTPUT}" PARENT_SCOPE)
endfunction()

set(SLANG_COMPILED_OUTPUTS "")

# screenquad vertex (shared by blur, bloom, and fxaa passes)
compile_slang_shader(fxaa.slang                vertMain   screenquad.vert.glsl)

# PBR
compile_slang_shader(pbr.slang                 vertMain   pbr.vert.glsl)
compile_slang_shader(pbr.slang                 fragMain   pbr.frag.glsl)

# Shadow map passes
compile_slang_shader(shadowmap.slang           vertMain     shadowmap.vert.glsl)
compile_slang_shader(shadowmap.slang           fragMainEVSM shadowmap_evsm.frag.glsl)
compile_slang_shader(shadowmap.slang           fragMain     shadowmap.frag.glsl)

# Shadow blur (Dual Kawase, EVSM RG channels)
compile_slang_shader(blur.slang                downFragMain      blur.frag.glsl)
compile_slang_shader(blur.slang                upFragMain        blur_up.frag.glsl)

# Bloom pipeline
compile_slang_shader(bloom.slang               extractFragMain   bloom_extract.frag.glsl)
compile_slang_shader(bloom.slang               downFragMain      bloom_down.frag.glsl)
compile_slang_shader(bloom.slang               upFragMain        bloom_up.frag.glsl)
compile_slang_shader(bloom.slang               compositeFragMain bloom_composite.frag.glsl)

# FXAA
compile_slang_shader(fxaa.slang                fragMain   fxaa.frag.glsl)


# 2D / UI
compile_slang_shader(quad.slang                vertMain   quad.vert.glsl)
compile_slang_shader(quad.slang                fragMain   quad.frag.glsl)
compile_slang_shader(sprite.slang              vertMain      sprite.vert.glsl)
compile_slang_shader(sprite.slang              fragMain      sprite.frag.glsl)
compile_slang_shader(sprite.slang              textFragMain  text.frag.glsl)

# Debug light cube
compile_slang_shader(cube.slang                vertMain   cube.vert.glsl)
compile_slang_shader(cube.slang                fragMain   cube.frag.glsl)

add_custom_target(compile_shaders ALL
    DEPENDS ${SLANG_COMPILED_OUTPUTS}
)

add_subdirectory(game)
