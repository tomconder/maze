# Game

file(GLOB HEADER_FILES LIST_DIRECTORIES false src/*.h)

file(GLOB SOURCE_FILES LIST_DIRECTORIES false src/*.cpp)

file(GLOB ASSETS LIST_DIRECTORIES false assets/*)

if (EMSCRIPTEN)
  message(STATUS "Emscripten: Building game for Emscripten")

  # 01/2022 - use fetchcontent since conan's support for emscripten on windows is poor
  include(FetchContent)

  # glm
  fetchcontent_declare(
          glm
          GIT_REPOSITORY https://github.com/g-truc/glm.git
          GIT_TAG 06ed280db4e274fa5e1f36d5ea4f7dfd654ff9b0
  )
  fetchcontent_makeavailable(glm)
  if (NOT glm_POPULATED)
    fetchcontent_populate(glm)
  endif ()

  # assimp
  fetchcontent_declare(
          assimp
          GIT_REPOSITORY https://github.com/assimp/assimp.git
          GIT_TAG 6841397798051919a23a3fbe1405fd934fb5f39c
  )

  fetchcontent_getproperties(assimp)
  string(TOLOWER "assimp" lcName)
  if (NOT ${lcName}_POPULATED)
    fetchcontent_populate(assimp)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error -Wno-deprecated-non-prototype -Wno-unused-but-set-variable")
    set(BUILD_SHARED_LIBS FALSE CACHE INTERNAL "override Assimp flags" FORCE)

    # turn off assimp options to improve build time and reduce size
    set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT FALSE CACHE INTERNAL "override Assimp flags" FORCE)
    set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE INTERNAL "override Assimp flags" FORCE)
    set(ASSIMP_BUILD_OBJ_IMPORTER TRUE CACHE INTERNAL "override Assimp flags" FORCE)
    set(ASSIMP_BUILD_TESTS OFF CACHE INTERNAL "override Assimp flags" FORCE)
    set(ASSIMP_DOUBLE_PRECISION ON CACHE INTERNAL "override Assimp flags" FORCE)
    set(ASSIMP_IGNORE_GIT_HASH ON CACHE INTERNAL "override Assimp flags" FORCE)
    set(ASSIMP_INSTALL_PDB FALSE CACHE INTERNAL "override Assimp flags" FORCE)
    set(ASSIMP_NO_EXPORT ON CACHE INTERNAL "override Assimp flags" FORCE)

    add_subdirectory(${${lcName}_SOURCE_DIR} ${${lcName}_BINARY_DIR})
  endif ()

  set(CMAKE_EXECUTABLE_SUFFIX ".html")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O2")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_SDL=2")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_FREETYPE=1")

  add_executable(game ${SOURCE_FILES} ${HEADER_FILES})
  set_target_properties(game PROPERTIES OUTPUT_NAME "maze")

  set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/html)
  set(ASSETS_BUILD_PATH ${PROJECT_SOURCE_DIR}/build)

  set(TARGET_LINK_PROPERTIES "-std=c++17 -O2 -s ALLOW_MEMORY_GROWTH=1")
  set(TARGET_LINK_PROPERTIES "${TARGET_LINK_PROPERTIES} -s USE_SDL=2 -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2")
  set(TARGET_LINK_PROPERTIES "${TARGET_LINK_PROPERTIES} -s USE_FREETYPE=1")
  set(TARGET_LINK_PROPERTIES "${TARGET_LINK_PROPERTIES} --preload-file ${ASSETS_BUILD_PATH}/assets@assets")
  set_target_properties(game PROPERTIES LINK_FLAGS ${TARGET_LINK_PROPERTIES})

  target_include_directories(game PUBLIC src)
  target_include_directories(game PUBLIC ${PROJECT_BINARY_DIR})
  target_include_directories(game PUBLIC ${PROJECT_SOURCE_DIR}/3rdparty/stb_image/src/stb_image.h)
  target_include_directories(game PUBLIC ${PROJECT_SOURCE_DIR}/3rdparty/stb_image/src/stb_image.cpp)

  target_link_libraries(game PRIVATE assimp)
  target_link_libraries(game PRIVATE glad)
  target_link_libraries(game PRIVATE glm::glm)
  target_link_libraries(game PRIVATE sponge)
  target_link_libraries(game PRIVATE stb_image)
  target_link_libraries(game PRIVATE spdlog)

  add_custom_command(
          TARGET game PRE_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_SOURCE_DIR}/build/assets
          COMMENT "Copying assets" VERBATIM
          DEPENDS ${ASSETS})
else ()
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
          freetype/2.12.1
          glm/0.9.9.8
          sdl/2.26.1
          zlib/1.2.13
          GENERATORS cmake_find_package)

  conan_cmake_autodetect(settings)

  conan_cmake_install(PATH_OR_REFERENCE .
          CMAKE_TARGETS
          BUILD missing
          REMOTE conancenter
          SETTINGS ${settings})

  set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR})
  set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/build)

  find_package(Freetype REQUIRED)
  find_package(SDL2 REQUIRED)
  find_package(assimp REQUIRED)
  find_package(glm REQUIRED)

  add_executable(game ${SOURCE_FILES} ${HEADER_FILES})
  set_target_properties(game PROPERTIES OUTPUT_NAME "maze")

  target_compile_features(game PRIVATE cxx_std_17)

  if (MSVC)
    target_compile_options(game PUBLIC "/Zi")
    target_link_options(game PUBLIC "/INCREMENTAL")
  endif ()

  target_include_directories(game PUBLIC src)
  target_include_directories(game PUBLIC ${PROJECT_BINARY_DIR})

  target_link_libraries(game PUBLIC glad)
  target_link_libraries(game PUBLIC sponge)
  target_link_libraries(game PRIVATE Freetype::Freetype)
  target_link_libraries(game PRIVATE SDL2::SDL2)
  target_link_libraries(game PRIVATE assimp::assimp)
  target_link_libraries(game PRIVATE glm::glm)
  target_link_libraries(game PRIVATE stb_image)
  target_link_libraries(game PRIVATE spdlog)

  install(TARGETS game
          BUNDLE DESTINATION bin
          RUNTIME DESTINATION bin
          LIBRARY DESTINATION lib
          ARCHIVE DESTINATION lib
          FRAMEWORK DESTINATION lib)

  add_custom_command(
          TARGET game POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_SOURCE_DIR}/build/assets
          COMMENT "Copying assets to ${CMAKE_SOURCE_DIR}/build/assets" VERBATIM
          DEPENDS ${ASSETS})
endif ()
