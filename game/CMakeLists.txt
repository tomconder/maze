include(GNUInstallDirs)

configure_file(bundle/resources/version.h.in include/version.h)

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

    configure_file(bundle/resources/maze.rc.in maze.rc)
endif ()

file(
        GLOB HEADER_FILES
        LIST_DIRECTORIES false
        src/*.hpp
        src/ui/*.hpp)

file(
        GLOB SOURCE_FILES
        LIST_DIRECTORIES false
        src/*.cpp
        src/ui/*.cpp)

file(
        GLOB ASSETS
        LIST_DIRECTORIES false
        assets/*)

find_package(absl CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/maze)

find_package(SDL2 CONFIG REQUIRED)

if (WIN32)
    set_source_files_properties(maze.rc PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
    add_executable(game WIN32 ${SOURCE_FILES} ${HEADER_FILES} maze.rc)
    target_include_directories(game PRIVATE bundle/icons)
elseif (APPLE)
    set(MACOSX_BUNDLE_ICON_FILE maze.icns)
    set(MACOSX_BUNDLE_BUNDLE_NAME ${CMAKE_PROJECT_NAME})
    set(MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
    set(MACOSX_BUNDLE_LONG_VERSION_STRING ${PROJECT_VERSION})
    set(MACOSX_BUNDLE_COPYRIGHT ${COPYRIGHT})

    set(app_icon_macos "bundle/icons/maze.icns")
    set_source_files_properties(
            ${app_icon_macos} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources"
            SKIP_PRECOMPILE_HEADERS ON)
    add_executable(game MACOSX_BUNDLE ${SOURCE_FILES} ${HEADER_FILES}
            ${app_icon_macos})
else ()
    add_executable(game ${SOURCE_FILES} ${HEADER_FILES})
endif ()

set_target_properties(game PROPERTIES OUTPUT_NAME "maze" CXX_STANDARD 23)

if (MSVC)
    # incremental builds
    target_compile_options(game PUBLIC $<$<NOT:$<CONFIG:RELEASE>>:/Zi>)
    target_link_options(game PUBLIC $<IF:$<CONFIG:RELEASE>,/INCREMENTAL:NO,/INCREMENTAL>)

    # hardening
    message(STATUS "Applying Windows hardening support")
    target_compile_options(game PUBLIC $<$<NOT:$<CONFIG:DEBUG>>:/DYNAMICBASE /guard:cf>)
    target_link_options(game PUBLIC "/NXCOMPAT /CETCOMPAT")
endif ()

target_include_directories(game PUBLIC src)
target_include_directories(game PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/include")

target_link_libraries(game PRIVATE $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main> $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>)
target_link_libraries(game PRIVATE absl::flat_hash_map)
target_link_libraries(game PRIVATE glm::glm)
target_link_libraries(game PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(game PRIVATE sponge)

if (APPLE)
    add_custom_command(
            TARGET game
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_BUNDLE_CONTENT_DIR:game>/Resources
            COMMENT "Copying assets to bundle content folder"
            VERBATIM DEPENDS ${ASSETS})

    install(TARGETS game BUNDLE DESTINATION .)

    set(APPS "$<TARGET_BUNDLE_DIR:game>")

    install(CODE "
        set(BU_CHMOD_BUNDLE_ITEMS ON)
        include(BundleUtilities)
        fixup_bundle(\"${APPS}\" \"${LIBS}\" \"${DIRS}\"\)"
            COMPONENT Runtime
    )

    set(CPACK_GENERATOR "DRAGNDROP")
    include(CPack)
else ()
    add_custom_command(
            TARGET game
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:game>/assets
            COMMENT "Copying assets to target folder"
            VERBATIM DEPENDS ${ASSETS})

    install(TARGETS game RUNTIME DESTINATION .)
    install(FILES $<TARGET_RUNTIME_DLLS:game> DESTINATION .)
    install(DIRECTORY assets DESTINATION .)
endif ()
