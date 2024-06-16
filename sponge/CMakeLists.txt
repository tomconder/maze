# Sponge Engine
include(GNUInstallDirs)

file(
        GLOB HEADER_FILES
        LIST_DIRECTORIES false
        src/*.hpp
        src/core/*.hpp
        src/event/*.hpp
        src/imgui/*.hpp
        src/input/*.hpp
        src/layer/*.hpp
        src/logging/*.hpp
        src/renderer/*.hpp
        src/scene/*.hpp)

file(
        GLOB SOURCE_FILES
        LIST_DIRECTORIES false
        src/core/*.cpp
        src/event/*.cpp
        src/imgui/*.cpp
        src/input/*.cpp
        src/layer/*.cpp
        src/logging/*.cpp
        src/renderer/*.cpp
        src/scene/*.cpp)

# OpenGL
message(STATUS "Including OpenGL platform files")
file(GLOB OPENGL_HEADERS LIST_DIRECTORIES false src/platform/opengl/*.hpp)
list(APPEND HEADER_FILES ${OPENGL_HEADERS})
file(GLOB OPENGL_SOURCES LIST_DIRECTORIES false src/platform/opengl/*.cpp)
list(APPEND SOURCE_FILES ${OPENGL_SOURCES})

# SDL
message(STATUS "Including SDL platform files")
file(
        GLOB SDL_HEADERS
        LIST_DIRECTORIES false
        src/platform/sdl/*.hpp
        src/platform/sdl/imgui/*.hpp
        src/platform/sdl/input/*.hpp
        src/platform/sdl/logging/*.hpp
)
list(APPEND HEADER_FILES ${SDL_HEADERS})
file(
        GLOB SDL_SOURCES
        LIST_DIRECTORIES false
        src/platform/sdl/*.cpp
        src/platform/sdl/imgui/*.cpp
        src/platform/sdl/input/*.cpp
        src/platform/sdl/logging/*.cpp
)
list(APPEND SOURCE_FILES ${SDL_SOURCES})

if (APPLE)
    message(STATUS "Including OSX platform files")
    file(GLOB OSX_HEADERS src/platform/osx/*.hpp)
    list(APPEND HEADER_FILES ${OSX_HEADERS})
    file(GLOB OSX_SOURCES src/platform/osx/*.cpp)
    list(APPEND SOURCE_FILES ${OSX_SOURCES})
endif ()

if (MSVC)
    message(STATUS "Including Windows platform files")
    file(GLOB WIN_HEADERS src/platform/windows/*.hpp)
    list(APPEND HEADER_FILES ${WIN_HEADERS})
    file(GLOB WIN_SOURCES src/platform/windows/*.cpp)
    list(APPEND SOURCE_FILES ${WIN_SOURCES})
endif ()

if (UNIX AND NOT APPLE)
    message(STATUS "Including Linux platform files")
    file(GLOB LINUX_HEADERS src/platform/linux/*.hpp)
    list(APPEND HEADER_FILES ${LINUX_HEADERS})
    file(GLOB LINUX_SOURCES src/platform/linux/*.cpp)
    list(APPEND SOURCE_FILES ${LINUX_SOURCES})
endif ()

find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(meshoptimizer CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)

find_package(OpenGL REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(Stb REQUIRED)

add_library(sponge ${HEADER_FILES} ${SOURCE_FILES})
add_library(sponge::sponge ALIAS sponge)

# pre-compiled header support
#message(STATUS "Compiling using pre-compiled header support")
#list(APPEND SOURCE_FILES src/spongepch.cpp)
#target_precompile_headers(sponge PUBLIC src/spongepch.hpp)
#set(CMAKE_PCH_INSTANTIATE_TEMPLATES ON)

if (MSVC)
    # hardening
    target_compile_options(sponge PUBLIC $<$<NOT:$<CONFIG:DEBUG>>:/DYNAMICBASE /guard:cf>)
    target_link_options(sponge PUBLIC "/NXCOMPAT /CETCOMPAT")

    # properly report what c++ version is being used
    target_compile_options(sponge INTERFACE /Zc:__cplusplus)

    # fast math and SIMD support
    # https://learn.microsoft.com/en-us/cpp/build/reference/fp-specify-floating-point-behavior?view=msvc-170#fast
    target_compile_options(sponge INTERFACE $<$<CONFIG:RELEASE>:/fp:fast>)
else ()
    # See the implications here: https://stackoverflow.com/q/45685487
    target_compile_options(sponge INTERFACE $<$<CONFIG:RELEASE>:-Ofast>)
endif ()

set_target_properties(sponge PROPERTIES CXX_STANDARD 23)

target_include_directories(sponge PUBLIC src)
target_include_directories(sponge SYSTEM PRIVATE ${Stb_INCLUDE_DIR})

target_link_libraries(sponge PRIVATE $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main> $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>)
target_link_libraries(sponge PRIVATE OpenGL::GL)
target_link_libraries(sponge PRIVATE glad::glad)
target_link_libraries(sponge PRIVATE glm::glm-header-only)
target_link_libraries(sponge PRIVATE imgui::imgui)
target_link_libraries(sponge PRIVATE meshoptimizer::meshoptimizer)
target_link_libraries(sponge PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(sponge PRIVATE tinyobjloader::tinyobjloader)
