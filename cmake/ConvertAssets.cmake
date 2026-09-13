# Bakes the assets listed in assets/manifest.json to the engine format.
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
        COMMAND assetconv --manifest "${CMAKE_SOURCE_DIR}/assets/manifest.json"
                "${CMAKE_BINARY_DIR}/assets"
                "$<$<CONFIG:Release>:--no-line-directives>"
                --notices "${NOTICES_FILE}" ${LICENSE_ARGS}
        COMMENT "Converting the assets in assets/manifest.json"
        VERBATIM
        COMMAND_EXPAND_LISTS)

# Files under assets/ that must not deploy. Consumed by CopyAssets.cmake and
# by the install rules in game/CMakeLists.txt, both of which match a REGEX
# against the whole path.
#
# - Images and fonts: every one is a bake input, and must not ship beside what
#   it was baked into.
# - manifest.json: drives the bake, never read at run time.
# - License files of third-party art and fonts: joined into the notices file
#   next to the executable instead.
#
# CMake regexes have no case-insensitive flag, and Linux does not fold the path
# case, so each letter is spelled as a two-case class.
set(DEPLOY_EXCLUDE_REGEX
        "\\.([Pp][Nn][Gg]|[Jj][Pp][Ee]?[Gg]|[Tt][Tt][Ff]|[Oo][Tt][Ff])$|manifest\\.json$|/[Ll][Ii][Cc][Ee][Nn][CcSs][Ee][^/]*$")
