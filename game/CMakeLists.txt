# Game

file(GLOB HEADER_FILES LIST_DIRECTORIES false
        src/*.h)

file(GLOB SOURCE_FILES LIST_DIRECTORIES false
        src/*.cpp)

# 01/2022 - conan does not pre-build cygwin gcc so you must target the visual studio toolchain

if (NOT EXISTS "${CMAKE_BINARY_DIR}/game/conan.cmake")
  message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
  file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/0.18.1/game/conan.cmake"
          "${CMAKE_BINARY_DIR}/game/conan.cmake"
          TLS_VERIFY ON)
endif ()

include(${CMAKE_BINARY_DIR}/game/conan.cmake)

conan_cmake_configure(REQUIRES
        assimp/5.2.2
        easyloggingpp/9.97.0
        freetype/2.12.1
        glm/0.9.9.8
        sdl/2.26.0
        GENERATORS cmake_find_package)

conan_cmake_autodetect(settings)

conan_cmake_install(PATH_OR_REFERENCE .
        CMAKE_TARGETS
        BUILD missing
        REMOTE conancenter
        SETTINGS ${settings})

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR})

find_package(Freetype REQUIRED)
find_package(SDL2 REQUIRED)
find_package(assimp REQUIRED)
find_package(easyloggingpp REQUIRED)
find_package(glm REQUIRED)

add_executable(maze ${SOURCE_FILES} ${HEADER_FILES})

if (MSVC)
  target_compile_options(maze PUBLIC "/Zi")
  target_link_options(maze PUBLIC "/INCREMENTAL")
endif ()

target_include_directories(maze PUBLIC src)
target_include_directories(maze PUBLIC ${PROJECT_BINARY_DIR})
target_include_directories(maze PUBLIC ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/sponge/src/)

target_link_libraries(maze PUBLIC glad)
target_link_libraries(maze PUBLIC sponge)
target_link_libraries(maze PRIVATE Freetype::Freetype)
target_link_libraries(maze PRIVATE SDL2::SDL2)
target_link_libraries(maze PRIVATE assimp::assimp)
target_link_libraries(maze PRIVATE easyloggingpp::easyloggingpp)
target_link_libraries(maze PRIVATE glad)
target_link_libraries(maze PRIVATE glm::glm)

file(GLOB ASSETS LIST_DIRECTORIES false assets/*)

file(GLOB SHADERS LIST_DIRECTORIES false
        src/shaders/*.frag
        src/shaders/*.vert)

message(STATUS "^^ copying assets")

install(TARGETS maze
        BUNDLE DESTINATION bin
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FRAMEWORK DESTINATION lib)

add_custom_command(
        TARGET maze POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_SOURCE_DIR}/build/assets
        COMMENT "Copying assets" VERBATIM)
