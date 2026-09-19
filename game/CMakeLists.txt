include(GNUInstallDirs)

configure_file(bundle/resources/version.hpp.in include/version.hpp)

file(
        GLOB HEADER_FILES
        CONFIGURE_DEPENDS
        LIST_DIRECTORIES false
        src/*.hpp src/layer/*.hpp src/layer/imgui/*.hpp src/scene/*.hpp
        src/thread/*.hpp src/ui/*.hpp)

file(
        GLOB SOURCE_FILES
        CONFIGURE_DEPENDS
        LIST_DIRECTORIES false
        src/*.cpp src/layer/*.cpp src/scene/*.cpp src/ui/*.cpp)

file(
        GLOB ASSETS
        LIST_DIRECTORIES false
        ${CMAKE_SOURCE_DIR}/assets/*)

if (ENABLE_IMGUI)
    file(
            GLOB IMGUI_SOURCES
            LIST_DIRECTORIES false
            src/layer/imgui/*.cpp)
    list(APPEND SOURCE_FILES ${IMGUI_SOURCES})
endif ()

find_package(fkYAML CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(yoga CONFIG REQUIRED)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/maze)

find_package(glfw3 CONFIG REQUIRED)
if (ENABLE_IMGUI)
    find_package(imgui CONFIG REQUIRED)
endif ()
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
    set(MACOSX_BUNDLE_GUI_IDENTIFIER co.itstom.Maze)
    set(MACOSX_DEPLOYMENT_TARGET "10.9")

    set(app_icon_macos "bundle/icons/maze.icns")
    set_source_files_properties(${app_icon_macos}
            PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    add_executable(game MACOSX_BUNDLE ${SOURCE_FILES} ${HEADER_FILES}
            ${app_icon_macos})
else ()
    add_executable(game ${SOURCE_FILES} ${HEADER_FILES})
endif ()

# Licenses of the third-party code compiled into maze.exe. Asset licenses are
# listed in assets/manifest.yaml. A missing file fails the configure, so a
# renamed port cannot drop a notice.
set(VCPKG_SHARE "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share")
set(THIRD_PARTY_LICENSES
        # The fkYAML port spells its share directory fkYAML on Windows and
        # fkyaml on Linux, so the path is taken from where find_package landed
        # rather than spelled out. The copyright sits beside the config file.
        "fkYAML=${fkYAML_DIR}/copyright"
        "fmt=${VCPKG_SHARE}/fmt/copyright"
        "glad=${CMAKE_SOURCE_DIR}/sponge/deps/glad2/copyright.txt"
        "glfw=${VCPKG_SHARE}/glfw3/copyright"
        "glm=${VCPKG_SHARE}/glm/copyright"
        "miniaudio=${CMAKE_SOURCE_DIR}/sponge/deps/miniaudio/LICENSE"
        "nlohmann-json=${VCPKG_SHARE}/nlohmann-json/copyright"
        "spdlog=${VCPKG_SHARE}/spdlog/copyright"
        "yoga=${VCPKG_SHARE}/yoga/copyright")
if (ENABLE_IMGUI)
    list(APPEND THIRD_PARTY_LICENSES "imgui=${VCPKG_SHARE}/imgui/copyright")
endif ()
if (ENABLE_PROFILING)
    # The Tracy port installs to share/Tracy, so take the path from
    # find_package, as for fkYAML above.
    list(APPEND THIRD_PARTY_LICENSES "tracy=${Tracy_DIR}/copyright")
endif ()
foreach (LICENSE IN LISTS THIRD_PARTY_LICENSES)
    string(REGEX REPLACE "^[^=]*=" "" LICENSE_FILE "${LICENSE}")
    if (NOT EXISTS "${LICENSE_FILE}")
        message(FATAL_ERROR "License file not found: ${LICENSE_FILE}")
    endif ()
endforeach ()

if (APPLE)
    set(NOTICES_FILE
            "$<TARGET_BUNDLE_CONTENT_DIR:game>/Resources/THIRD-PARTY-NOTICES.txt")
else ()
    set(NOTICES_FILE "$<TARGET_FILE_DIR:game>/THIRD-PARTY-NOTICES.txt")
endif ()

include(${CMAKE_SOURCE_DIR}/cmake/ConvertAssets.cmake)

add_dependencies(game convert_assets)

# strip release build binaries (non-MSVC only; on MSVC, debug symbols live in PDB)
if (NOT MSVC)
    add_custom_command(TARGET game POST_BUILD COMMAND $<$<CONFIG:Release>:${CMAKE_STRIP}> $<$<CONFIG:Release>:$<TARGET_FILE:game>>)
endif ()

set_target_properties(game PROPERTIES OUTPUT_NAME "maze" CXX_STANDARD 23)

if (MSVC)
    # hardening
    message(STATUS "Applying Windows hardening support")
    target_compile_options(game PUBLIC $<$<NOT:$<CONFIG:DEBUG>>:/DYNAMICBASE
            /guard:cf>)

    # properly report what c++ version is being used
    target_compile_options(game PUBLIC /Zc:__cplusplus)

    # fast math and SIMD support
    # https://learn.microsoft.com/en-us/cpp/build/reference/fp-specify-floating-point-behavior?view=msvc-170#fast
    message(STATUS "Applying fast math and SIMD support")
    target_compile_options(game PUBLIC $<$<CONFIG:RELEASE>:/fp:fast>)

    # do not omit frame pointers for crash dumps
    target_compile_options(game PUBLIC /Oy-)

    # enable exception handling
    target_compile_options(game PUBLIC /EHsc)

    # enable warnings as errors
    target_compile_options(game PUBLIC /W4 /WX)

    # disable RTTI
    message(STATUS "Disable RTTI")
    target_compile_options(game PUBLIC $<$<COMPILE_LANGUAGE:CXX>:/GR->)
else ()
    # see the implications here: https://stackoverflow.com/q/45685487
    message(STATUS "Applying fast math and SIMD support")
    target_compile_options(game PUBLIC $<$<CONFIG:RELEASE>:-ffast-math>)

    # do not omit frame pointers for crash dumps
    target_compile_options(game PUBLIC -fno-omit-frame-pointer)

    # enable exception handling
    target_compile_options(game PUBLIC -fexceptions)

    # enable warnings as errors
    target_compile_options(game PUBLIC -Wall -Werror)

    # disable RTTI
    message(STATUS "Disable RTTI")
    target_compile_options(game PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)

    # enable separate sections per function and data item
    target_compile_options(game
            PUBLIC $<$<CONFIG:RELEASE>:-ffunction-sections>)
    target_compile_options(game PUBLIC $<$<CONFIG:RELEASE>:-fdata-sections>)

    # discard unused sections
    message(STATUS "Discard unused sections")
    target_link_options(
            game PUBLIC
            $<$<CONFIG:RELEASE>:$<$<CXX_COMPILER_ID:Clang,GNU>:-Wl,--gc-sections>>)
    target_link_options(
            game PUBLIC
            $<$<CONFIG:RELEASE>:$<$<CXX_COMPILER_ID:AppleClang>:-Wl,-dead_strip>>)
endif ()

target_include_directories(game PUBLIC src)
target_include_directories(game PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/include")

target_link_libraries(game PRIVATE fkYAML::fkYAML)
target_link_libraries(game PRIVATE sponge::sponge)
target_link_libraries(game PRIVATE yoga::yogacore)

if (CMAKE_SKIP_INSTALL_RULES)
    return()
endif ()

if (APPLE)
    # Always-run target (file(COPY) skips up-to-date files) so shader edits
    # deploy even when the game target does not relink.
    add_custom_target(
            copy_assets
            COMMAND ${CMAKE_COMMAND}
                -D DST_DIR=$<TARGET_BUNDLE_CONTENT_DIR:game>/Resources
                -D SRC_DIR=${CMAKE_SOURCE_DIR}/assets
                -D SRC_BAKED=${CMAKE_BINARY_DIR}/assets
                -D EXCLUDE_REGEX=${DEPLOY_EXCLUDE_REGEX}
                -P "${CMAKE_SOURCE_DIR}/cmake/CopyAssets.cmake"
            COMMENT "Copying assets to bundle content folder"
            VERBATIM)
    add_dependencies(copy_assets convert_assets)
    add_dependencies(game copy_assets)

    install(TARGETS game BUNDLE DESTINATION .)

    set(APPS "$<TARGET_BUNDLE_DIR:game>")

    install(
            CODE "
        set(BU_CHMOD_BUNDLE_ITEMS ON)
        include(BundleUtilities)
        fixup_bundle(\"${APPS}\" \"${LIBS}\" \"${DIRS}\"\)"
            COMPONENT Runtime)
else ()
    # Always-run target (file(COPY) skips up-to-date files) so shader edits
    # deploy even when the game target does not relink.
    add_custom_target(
            copy_assets
            COMMAND ${CMAKE_COMMAND}
                -D DST_DIR=$<TARGET_FILE_DIR:game>/assets
                -D SRC_DIR=${CMAKE_SOURCE_DIR}/assets
                -D SRC_BAKED=${CMAKE_BINARY_DIR}/assets
                -D EXCLUDE_REGEX=${DEPLOY_EXCLUDE_REGEX}
                -P "${CMAKE_SOURCE_DIR}/cmake/CopyAssets.cmake"
            COMMENT "Copying assets to target folder"
            VERBATIM)
    add_dependencies(copy_assets convert_assets)
    add_dependencies(game copy_assets)

    install(TARGETS game RUNTIME DESTINATION .)
    install(FILES $<TARGET_RUNTIME_DLLS:game> ${NOTICES_FILE} DESTINATION .)
    # Same script as the copy above, so the exclusions live in one place.
    # install(DIRECTORY) cannot drop the folders the exclusions leave empty.
    install(CODE "
        set(SRC_DIR [=[${CMAKE_SOURCE_DIR}/assets]=])
        set(DST_DIR \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/assets\")
        set(SRC_BAKED [=[${CMAKE_BINARY_DIR}/assets]=])
        set(EXCLUDE_REGEX [=[${DEPLOY_EXCLUDE_REGEX}]=])
        include([=[${CMAKE_SOURCE_DIR}/cmake/CopyAssets.cmake]=])")
endif ()
