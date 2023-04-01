# Sponge Engine

include(GNUInstallDirs)

file(
        GLOB HEADER_FILES
        LIST_DIRECTORIES false
        src/*.h
        src/core/*.h
        src/event/*.h
        src/platform/*.h
        src/platform/sdl/*.h
        src/renderer/*.h
        src/renderer/opengl/*.h
        src/scene/*.h)

file(
        GLOB SOURCE_FILES
        LIST_DIRECTORIES false
        src/core/*.cpp
        src/event/*.cpp
        src/platform/*.cpp
        src/platform/sdl/*.cpp
        src/renderer/*.cpp
        src/renderer/opengl/*.cpp
        src/scene/*.cpp)

if (MSVC)
  list(APPEND SOURCE_FILES src/spongepch.cpp)
endif ()

if (EMSCRIPTEN)
  find_package(glad CONFIG REQUIRED)
  find_package(Stb REQUIRED)
  find_package(glm CONFIG REQUIRED)
  find_package(spdlog CONFIG REQUIRED)
  find_package(tinyobjloader CONFIG REQUIRED)

  message(STATUS "Emscripten: Building sponge for Emscripten")

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_SDL=2")

  add_library(sponge ${HEADER_FILES} ${SOURCE_FILES})

  # pre-compiled header support
  message(STATUS "Compiling using pre-compiled header support")
  target_precompile_headers(sponge PUBLIC src/spongepch.h)
  set(CMAKE_PCH_INSTANTIATE_TEMPLATES ON)

  set(TARGET_LINK_PROPERTIES
          "${TARGET_LINK_PROPERTIES} -O2 -s ALLOW_MEMORY_GROWTH=1")
  set(TARGET_LINK_PROPERTIES
          "${TARGET_LINK_PROPERTIES} -s USE_SDL=2 -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2"
          )

  set_target_properties(sponge PROPERTIES LINK_FLAGS ${TARGET_LINK_PROPERTIES})

  target_include_directories(sponge PUBLIC src)
  target_include_directories(sponge PRIVATE ${Stb_INCLUDE_DIR})

  target_link_libraries(sponge PRIVATE glad::glad)
  target_link_libraries(sponge PRIVATE glm::glm)
  target_link_libraries(sponge PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
  target_link_libraries(sponge PRIVATE tinyobjloader::tinyobjloader)
else ()
  find_package(OpenGL REQUIRED)
  find_package(SDL2 CONFIG REQUIRED)
  find_package(Stb REQUIRED)
  find_package(glad CONFIG REQUIRED)
  find_package(glm CONFIG REQUIRED)
  find_package(spdlog CONFIG REQUIRED)
  find_package(tinyobjloader CONFIG REQUIRED)

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
  target_include_directories(sponge PRIVATE ${Stb_INCLUDE_DIR})

  target_link_libraries(sponge PRIVATE $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main> $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>)
  target_link_libraries(sponge PRIVATE OpenGL::GL)
  target_link_libraries(sponge PRIVATE glad::glad)
  target_link_libraries(sponge PRIVATE glm::glm)
  target_link_libraries(sponge PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
  target_link_libraries(sponge PRIVATE tinyobjloader::tinyobjloader)

  install(
          TARGETS sponge
          BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
          FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
          INCLUDES
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

  install(FILES ${HEADER_FILES} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif ()
