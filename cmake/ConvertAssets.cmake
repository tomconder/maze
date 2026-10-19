# Bakes the assets listed in assets/manifest.yaml to the engine format.
# Nothing is converted by globbing: the manifest is the only list.
#
# assetconv reads the manifest and converts everything in it. It skips an
# output newer than its sources, the manifest and assetconv itself, so the
# target runs on every build and an edit rebakes only what it touches.
#
# It also joins the asset licenses in the manifest and THIRD_PARTY_LICENSES
# into NOTICES_FILE, both set by game/CMakeLists.txt.
set(LICENSE_ARGS "")
foreach (LICENSE IN LISTS THIRD_PARTY_LICENSES)
    list(APPEND LICENSE_ARGS --license "${LICENSE}")
endforeach ()

add_custom_target(
        convert_assets ALL
        COMMAND assetconv --manifest "${CMAKE_SOURCE_DIR}/assets/manifest.yaml"
                "${CMAKE_BINARY_DIR}/assets"
                "$<$<CONFIG:Release>:--no-line-directives>"
                "$<$<CONFIG:Release,RelWithDebInfo,MinSizeRel>:--no-debug-assets>"
                --notices "${NOTICES_FILE}" ${LICENSE_ARGS}
        COMMENT "Converting the assets in assets/manifest.yaml"
        USES_TERMINAL
        VERBATIM
        COMMAND_EXPAND_LISTS)

# Files under assets/ that must not deploy. Consumed by CopyAssets.cmake and
# by the install rules in game/CMakeLists.txt, both of which match a REGEX
# against the whole path.
#
# - Images and fonts: every one is a bake input, and must not ship beside what
#   it was baked into.
# - manifest.yaml: drives the bake, never read at run time. Matched by name,
#   not by extension: assets/scenes/*.yaml is read at run time and ships.
# - License files of third-party art and fonts: joined into the notices file
#   next to the executable instead.
# - The mesh LOD test scene, in release builds: only the debug capture flags
#   load it, and its models are baked for debug builds only.
#
# CMake regexes have no case-insensitive flag, and Linux does not fold the path
# case, so each letter is spelled as a two-case class. The value holds a
# generator expression, so only commands and install(CODE) can use it.
set(DEPLOY_EXCLUDE_REGEX
        "\\.([Pp][Nn][Gg]|[Jj][Pp][Ee]?[Gg]|[Tt][Tt][Ff]|[Oo][Tt][Ff])$|manifest\\.yaml$|/[Ll][Ii][Cc][Ee][Nn][CcSs][Ee][^/]*$$<$<CONFIG:Release,RelWithDebInfo,MinSizeRel>:|/scenes/lodtest\\.yaml$>")
