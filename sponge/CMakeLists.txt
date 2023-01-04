# Sponge Engine

file(GLOB HEADER_FILES LIST_DIRECTORIES false
        src/*.h
        src/core/*.h
        src/platform/opengl/*.h
        src/renderer/*.h)

file(GLOB SOURCE_FILES LIST_DIRECTORIES false
        src/*.cpp
        src/core/*.cpp
        src/platform/opengl/*.cpp
        src/renderer/*.cpp)

# 01/2022 - conan does not pre-build cygwin gcc so you must target the visual studio toolchain

if (NOT EXISTS "${CMAKE_BINARY_DIR}/sponge/conan.cmake")
  message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
  file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/0.18.1/conan.cmake"
          "${CMAKE_BINARY_DIR}/sponge/conan.cmake"
          TLS_VERIFY ON)
endif ()

include(${CMAKE_BINARY_DIR}/sponge/conan.cmake)

conan_cmake_configure(REQUIRES
        assimp/5.2.2
        easyloggingpp/9.97.0
        freetype/2.12.1
        glm/0.9.9.8
        sdl/2.26.0
        stb/cci.20210910
        GENERATORS cmake_find_package)

conan_cmake_autodetect(settings)

conan_cmake_install(PATH_OR_REFERENCE .
        CMAKE_TARGETS
        OUTPUT_FOLDER ${CMAKE_CURRENT_BINARY_DIR}
        BUILD missing
        REMOTE conancenter
        SETTINGS ${settings})

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR})

find_package(Freetype REQUIRED)
find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(stb REQUIRED)
find_package(assimp REQUIRED)
find_package(easyloggingpp REQUIRED)
find_package(glm REQUIRED)

# for debugging
# include(CMakePrintHelpers)
# cmake_print_variables(SDL2_INCLUDE_DIR)
# cmake_print_variables(SDL2_LIBRARY)

add_library(sponge ${HEADER_FILES} ${SOURCE_FILES})

if (MSVC)
  target_compile_options(sponge PUBLIC "/Zi")
  target_link_options(sponge PUBLIC "/INCREMENTAL")
endif ()

target_include_directories(sponge PUBLIC src)

target_link_libraries(sponge PRIVATE Freetype::Freetype)
target_link_libraries(sponge PRIVATE OpenGL::GL)
target_link_libraries(sponge PRIVATE SDL2::SDL2)
target_link_libraries(sponge PRIVATE assimp::assimp)
target_link_libraries(sponge PRIVATE easyloggingpp::easyloggingpp)
target_link_libraries(sponge PRIVATE glad)
target_link_libraries(sponge PRIVATE glm::glm)

install(TARGETS sponge
        BUNDLE DESTINATION bin
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FRAMEWORK DESTINATION lib)

install(FILES ${HEADER_FILES} DESTINATION sponge)

add_custom_command(
        TARGET sponge POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders ${CMAKE_SOURCE_DIR}/build/assets/shaders
        COMMENT "Copying shaders" VERBATIM)

