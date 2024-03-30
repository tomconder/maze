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
        src/platform/opengl/*.hpp
        src/platform/sdl/*.hpp
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
        src/platform/opengl/*.cpp
        src/platform/sdl/*.cpp
        src/renderer/*.cpp
        src/scene/*.cpp)

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

find_package(absl CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(meshoptimizer CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(tuplet CONFIG REQUIRED)

find_package(OpenGL REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(Stb REQUIRED)

add_library(sponge ${HEADER_FILES} ${SOURCE_FILES})

# pre-compiled header support
message(STATUS "Compiling using pre-compiled header support")
list(APPEND SOURCE_FILES src/spongepch.cpp)
target_precompile_headers(sponge PUBLIC src/spongepch.hpp)
set(CMAKE_PCH_INSTANTIATE_TEMPLATES ON)

set_target_properties(sponge PROPERTIES CXX_STANDARD 23)

target_include_directories(sponge PUBLIC src)
target_include_directories(sponge SYSTEM PRIVATE ${Stb_INCLUDE_DIR})

target_link_libraries(sponge PRIVATE $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main> $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>)
target_link_libraries(sponge PRIVATE OpenGL::GL)
target_link_libraries(sponge PRIVATE absl::flat_hash_map)
target_link_libraries(sponge PRIVATE glad::glad)
target_link_libraries(sponge PRIVATE glm::glm-header-only)
target_link_libraries(sponge PRIVATE imgui::imgui)
target_link_libraries(sponge PRIVATE meshoptimizer::meshoptimizer)
target_link_libraries(sponge PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(sponge PRIVATE tinyobjloader::tinyobjloader)
target_link_libraries(sponge PRIVATE tuplet::tuplet)
