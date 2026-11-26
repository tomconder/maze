# Bakes the assets listed in assets/manifest.yaml to the engine format.
# Nothing is converted by globbing: the manifest is the only list.
#
# yeschef reads the manifest and converts everything in it. It skips an
# output newer than its sources, the manifest and yeschef itself, so the
# target runs on every build and an edit rebakes only what it touches.
#
# A saved font atlas that no longer matches its font fails the bake, except in
# Debug, which warns and uses the drawn atlas.
#
# It also joins the asset licenses in the manifest and THIRD_PARTY_LICENSES
# into NOTICES_FILE, both set by game/CMakeLists.txt.
#
# With MAZE_PACK_ASSETS, yeschef also writes the baked assets as pack files, and
# DEPLOY_BAKED_DIR, which game/CMakeLists.txt deploys, holds only the pack. Debug
# defaults to loose files, so a changed asset needs no new pack. The default
# comes from CMAKE_BUILD_TYPE, which a single-configuration generator sets.
set(PACK_DEFAULT ON)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(PACK_DEFAULT OFF)
endif ()
option(MAZE_PACK_ASSETS "Ship the baked assets as pack files" ${PACK_DEFAULT})

set(PACK_ARGS "")
set(DEPLOY_BAKED_DIR "${CMAKE_BINARY_DIR}/assets")
if (MAZE_PACK_ASSETS)
    set(PACK_ARGS --pack "${CMAKE_BINARY_DIR}/assets-pack")
    set(DEPLOY_BAKED_DIR "${CMAKE_BINARY_DIR}/assets-pack")
endif ()

set(LICENSE_ARGS "")
foreach (LICENSE IN LISTS THIRD_PARTY_LICENSES)
    list(APPEND LICENSE_ARGS --license "${LICENSE}")
endforeach ()

add_custom_target(
        convert_assets ALL
        COMMAND ${YESCHEF_COMMAND} --manifest "${CMAKE_SOURCE_DIR}/assets/manifest.yaml"
                "${CMAKE_BINARY_DIR}/assets"
                "$<$<CONFIG:Release>:--no-line-directives>"
                "$<$<CONFIG:Release,RelWithDebInfo,MinSizeRel>:--no-debug-assets>"
                "$<$<CONFIG:Debug>:--allow-stale-atlas>"
                --notices "${NOTICES_FILE}" ${LICENSE_ARGS} ${PACK_ARGS}
        COMMENT "Converting the assets in assets/manifest.yaml"
        VERBATIM
        COMMAND_EXPAND_LISTS)
add_dependencies(convert_assets yeschef)

# Files under assets/ that must not deploy. Consumed by CopyAssets.cmake and
# by the install rules in game/CMakeLists.txt, both of which match a REGEX
# against the whole path.
#
# - Images (HDR too) and fonts: every one is a bake input, and must not ship
#   beside what it was baked into. The same for a saved font atlas, a .ktx2 in
#   fonts/. Other .ktx2 files are baked outputs, which must stay.
# - manifest.yaml: drives the bake, never read at run time.
# - License files of third-party art and fonts: joined into the notices file
#   next to the executable instead.
#
# CMake regexes have no case-insensitive flag, and Linux does not fold the path
# case, so each letter is spelled as a two-case class.
set(DEPLOY_EXCLUDE_REGEX
        "\\.([Pp][Nn][Gg]|[Jj][Pp][Ee]?[Gg]|[Tt][Tt][Ff]|[Oo][Tt][Ff]|[Hh][Dd][Rr])$|/[Ff][Oo][Nn][Tt][Ss]/[^/]*\\.[Kk][Tt][Xx]2$|manifest\\.yaml$|/[Ll][Ii][Cc][Ee][Nn][CcSs][Ee][^/]*$")
