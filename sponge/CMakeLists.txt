# Sponge Engine

include(GNUInstallDirs)

file(GLOB HEADER_FILES LIST_DIRECTORIES false
        src/*.h
        src/core/*.h
        src/event/*.h
        src/platform/*.h
        src/platform/sdl/*.h
        src/renderer/*.h
        src/renderer/opengl/*.h
        src/scene/*.h)

file(GLOB SOURCE_FILES LIST_DIRECTORIES false
        src/core/*.cpp
        src/event/*.cpp
        src/platform/*.cpp
        src/platform/sdl/*.cpp
        src/renderer/*.cpp
        src/renderer/opengl/*.cpp
        src/scene/*.cpp)

if (MSVC)
  list(APPEND SOURCES src/spongepch.cpp)
endif ()

if (EMSCRIPTEN)
  message(STATUS "Emscripten: Building sponge for Emscripten")

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_SDL=2")

  add_library(sponge ${HEADER_FILES} ${SOURCE_FILES})

  # pre-compiled header support
  message(STATUS "Compiling using pre-compiled header support")
  target_precompile_headers(sponge PUBLIC src/spongepch.h)
  set(CMAKE_PCH_INSTANTIATE_TEMPLATES ON)

  set(TARGET_LINK_PROPERTIES "${TARGET_LINK_PROPERTIES} -O2 -s ALLOW_MEMORY_GROWTH=1")
  set(TARGET_LINK_PROPERTIES "${TARGET_LINK_PROPERTIES} -s USE_SDL=2 -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2")

  set_target_properties(sponge PROPERTIES LINK_FLAGS ${TARGET_LINK_PROPERTIES})

  target_include_directories(sponge PUBLIC src)

  target_link_libraries(sponge PRIVATE glad)
  target_link_libraries(sponge PRIVATE glm::glm)
  target_link_libraries(sponge PRIVATE spdlog)
  target_link_libraries(sponge PRIVATE stb_image)
  target_link_libraries(sponge PRIVATE tinyobjloader)
else ()
  # 01/2022 - conan does not pre-build cygwin gcc so you must target the visual studio toolchain

  if (NOT EXISTS "${CMAKE_BINARY_DIR}/sponge/conan.cmake")
    message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
    file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/0.18.1/conan.cmake"
            "${CMAKE_BINARY_DIR}/sponge/conan.cmake"
            TLS_VERIFY ON)
  endif ()

  include(${CMAKE_BINARY_DIR}/sponge/conan.cmake)

  conan_cmake_configure(REQUIRES
          sdl/2.26.1
          GENERATORS cmake_find_package)

  conan_cmake_autodetect(settings)

  conan_cmake_install(PATH_OR_REFERENCE .
          CMAKE_TARGETS
          OUTPUT_FOLDER ${CMAKE_CURRENT_BINARY_DIR}
          BUILD missing
          REMOTE conancenter
          SETTINGS ${settings})

  set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR})

  find_package(OpenGL REQUIRED)
  find_package(SDL2 REQUIRED)

  # for debugging
  # include(CMakePrintHelpers)
  # cmake_print_variables(SDL2_INCLUDE_DIR)
  # cmake_print_variables(SDL2_LIBRARY)

  add_library(sponge ${HEADER_FILES} ${SOURCE_FILES})

  # pre-compiled header support
  message(STATUS "Compiling using pre-compiled header support")
  target_precompile_headers(sponge PUBLIC src/spongepch.h)
  set(CMAKE_PCH_INSTANTIATE_TEMPLATES ON)

  if (MSVC)
    target_compile_options(sponge PUBLIC "/Zi")
    target_link_options(sponge PUBLIC "/INCREMENTAL")
  endif ()

  target_include_directories(sponge PUBLIC src)

  target_link_libraries(sponge PRIVATE OpenGL::GL)
  target_link_libraries(sponge PRIVATE SDL2::SDL2)
  target_link_libraries(sponge PRIVATE glad)
  target_link_libraries(sponge PRIVATE glm::glm)
  target_link_libraries(sponge PRIVATE spdlog)
  target_link_libraries(sponge PRIVATE stb_image)
  target_link_libraries(sponge PRIVATE tinyobjloader)

  install(TARGETS sponge
          BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
          FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
          INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

  install(FILES ${HEADER_FILES} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif ()
