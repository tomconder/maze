include(GNUInstallDirs)

configure_file(${CMAKE_SOURCE_DIR}/cmake/version.h.in version.h)

if (WIN32)
    # accommodate missing version number values
    string(APPEND VERSION ${PROJECT_VERSION_MAJOR})
    if (PROJECT_VERSION_MINOR)
        string(APPEND VERSION ",${PROJECT_VERSION_MINOR}")
    else ()
        string(APPEND VERSION ",0")
    endif ()
    if (PROJECT_VERSION_PATCH)
        string(APPEND VERSION ",${PROJECT_VERSION_PATCH}")
    else ()
        string(APPEND VERSION ",0")
    endif ()
    if (PROJECT_VERSION_TWEAK)
        string(APPEND VERSION ",${PROJECT_VERSION_TWEAK}")
    else ()
        string(APPEND VERSION ",0")
    endif ()

    configure_file(${CMAKE_SOURCE_DIR}/cmake/maze.rc.in maze.rc)
endif ()

file(
        GLOB HEADER_FILES
        LIST_DIRECTORIES false
        src/*.h)

file(
        GLOB SOURCE_FILES
        LIST_DIRECTORIES false
        src/*.cpp)

file(
        GLOB ASSETS
        LIST_DIRECTORIES false
        assets/*)

find_package(glm CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

if (EMSCRIPTEN)
    message(STATUS "Emscripten: Building game for Emscripten")

    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_SDL=2")

    add_executable(game ${SOURCE_FILES} ${HEADER_FILES})
    set_target_properties(game PROPERTIES OUTPUT_NAME "maze")

    set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/maze/html)
    set(ASSETS_BUILD_PATH ${CMAKE_BINARY_DIR}/maze)

    set(TARGET_LINK_PROPERTIES "-std=c++17 -O3 -s ALLOW_MEMORY_GROWTH=1")
    set(TARGET_LINK_PROPERTIES
            "${TARGET_LINK_PROPERTIES} -s USE_SDL=2 -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2"
            )
    set(TARGET_LINK_PROPERTIES
            "${TARGET_LINK_PROPERTIES} --preload-file ${CMAKE_CURRENT_SOURCE_DIR}/assets@assets"
            )
    set_target_properties(game PROPERTIES LINK_FLAGS ${TARGET_LINK_PROPERTIES})

    target_include_directories(game PUBLIC src)
    target_include_directories(game PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")

    target_link_libraries(game PRIVATE glm::glm)
    target_link_libraries(game PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
    target_link_libraries(game PRIVATE sponge)
else ()
    set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/maze)

    find_package(SDL2 CONFIG REQUIRED)

    if (WIN32)
        set_source_files_properties(maze.rc PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
        add_executable(game WIN32 ${SOURCE_FILES} ${HEADER_FILES} maze.rc)
        target_include_directories(game PRIVATE assets/icons)
    elseif (APPLE)
        set(MACOSX_BUNDLE_ICON_FILE maze.icns)
        set(MACOSX_BUNDLE_BUNDLE_NAME ${CMAKE_PROJECT_NAME})
        set(MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
        set(MACOSX_BUNDLE_LONG_VERSION_STRING ${PROJECT_VERSION})
        set(MACOSX_BUNDLE_COPYRIGHT ${COPYRIGHT})

        set(app_icon_macos "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/maze.icns")
        set_source_files_properties(
                ${app_icon_macos} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources"
                SKIP_PRECOMPILE_HEADERS ON)
        add_executable(game MACOSX_BUNDLE ${SOURCE_FILES} ${HEADER_FILES}
                ${app_icon_macos} assets)
    else ()
        add_executable(game ${SOURCE_FILES} ${HEADER_FILES})
    endif ()

    set_target_properties(game PROPERTIES OUTPUT_NAME "maze")

    target_compile_features(game PRIVATE cxx_std_17)

    if (MSVC)
        target_compile_options(game PUBLIC "/Zi")
        target_link_options(game PUBLIC "/INCREMENTAL")
    endif ()

    target_include_directories(game PUBLIC src)
    target_include_directories(game PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")

    target_link_libraries(game PRIVATE $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main> $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>)
    target_link_libraries(game PRIVATE glm::glm)
    target_link_libraries(game PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
    target_link_libraries(game PUBLIC sponge)

    # CMake provides a standalone bundle application to fix up shared libraries
    # found in the app bundle See
    # https://cmake.org/cmake/help/latest/module/BundleUtilities.html

    add_custom_command(
            TARGET game
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:game>/assets
            COMMENT "Copying assets to ${CMAKE_SOURCE_DIR}/build/assets"
            VERBATIM DEPENDS ${ASSETS})
endif ()
