include(GNUInstallDirs)

configure_file(bundle/resources/version.hpp.in include/version.hpp)

file(
        GLOB HEADER_FILES
        LIST_DIRECTORIES false
        src/*.hpp
        src/layer/*.hpp
        src/scene/*.hpp
        src/ui/*.hpp)

file(
        GLOB SOURCE_FILES
        LIST_DIRECTORIES false
        src/*.cpp
        src/layer/*.cpp
        src/scene/*.cpp
        src/ui/*.cpp)

file(
        GLOB ASSETS
        LIST_DIRECTORIES false
        ${CMAKE_SOURCE_DIR}/assets/*)

find_package(glm CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/maze)

find_package(glfw3 CONFIG REQUIRED)
if (ENABLE_PROFILING)
    find_package(Tracy CONFIG REQUIRED)
endif ()

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
            ${app_icon_macos} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    add_executable(game MACOSX_BUNDLE ${SOURCE_FILES} ${HEADER_FILES}
            ${app_icon_macos})
else ()
    add_executable(game ${SOURCE_FILES} ${HEADER_FILES})
endif ()

set_target_properties(game PROPERTIES OUTPUT_NAME "maze" CXX_STANDARD 23)

if (MSVC)
    # hardening
    message(STATUS "Applying Windows hardening support")
    target_compile_options(game PUBLIC $<$<NOT:$<CONFIG:DEBUG>>:/DYNAMICBASE /guard:cf>)
    target_link_options(game PUBLIC "/NXCOMPAT /CETCOMPAT")

    # properly report what c++ version is being used
    target_compile_options(game INTERFACE /Zc:__cplusplus)

    # fast math and SIMD support
    # https://learn.microsoft.com/en-us/cpp/build/reference/fp-specify-floating-point-behavior?view=msvc-170#fast
    message(STATUS "Applying fast math and SIMD support")
    target_compile_options(game INTERFACE $<$<CONFIG:RELEASE>:/fp:fast>)
else ()
    # See the implications here: https://stackoverflow.com/q/45685487
    message(STATUS "Applying fast math and SIMD support")
    target_compile_options(game INTERFACE $<$<CONFIG:RELEASE>:-Ofast>)
endif ()

target_include_directories(game PUBLIC src)
target_include_directories(game PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/include")

target_link_libraries(game PRIVATE glfw)
target_link_libraries(game PRIVATE glm::glm)
target_link_libraries(game PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(game PRIVATE sponge::sponge)
if (ENABLE_PROFILING)
    target_link_libraries(game PRIVATE Tracy::TracyClient)
endif ()

if (APPLE)
    add_custom_command(
            TARGET game
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets $<TARGET_BUNDLE_CONTENT_DIR:game>/Resources
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
else ()
    add_custom_command(
            TARGET game
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:game>/assets
            COMMENT "Copying assets to target folder"
            VERBATIM DEPENDS ${ASSETS})

    install(TARGETS game RUNTIME DESTINATION .)
    install(FILES $<TARGET_RUNTIME_DLLS:game> DESTINATION .)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets DESTINATION .)
endif ()
