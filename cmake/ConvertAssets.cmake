# Bakes the assets listed in assets/manifest.json to the engine format.
# Nothing is converted by globbing: the manifest is the only list.
#
# One custom command per asset, collected into a single always-checked target.

set(MANIFEST "${CMAKE_SOURCE_DIR}/assets/manifest.json")
set(ASSET_SOURCE_DIR "${CMAKE_SOURCE_DIR}/assets")
set(ASSET_OUTPUT_DIR "${CMAKE_BINARY_DIR}/assets")
set(ASSET_FORMAT_HEADER "${CMAKE_SOURCE_DIR}/sponge/src/scene/assetformat.hpp")

file(READ "${MANIFEST}" MANIFEST_JSON)
string(JSON MODEL_COUNT LENGTH "${MANIFEST_JSON}" models)
message(STATUS "Asset manifest lists ${MODEL_COUNT} model(s)")

set(CONVERTED_ASSETS "")

math(EXPR LAST_MODEL "${MODEL_COUNT} - 1")
foreach (INDEX RANGE ${LAST_MODEL})
    string(JSON ENTRY GET "${MANIFEST_JSON}" models ${INDEX})
    string(JSON SOURCE GET "${ENTRY}" source)
    string(JSON OUTPUT GET "${ENTRY}" output)

    set(INPUT_PATH "${ASSET_SOURCE_DIR}/${SOURCE}")
    set(OUTPUT_PATH "${ASSET_OUTPUT_DIR}/${OUTPUT}")

    # DEPENDS on the manifest and the format header as well as the source, so
    # a version bump or a manifest edit rebakes without a clean.
    add_custom_command(
            OUTPUT "${OUTPUT_PATH}"
            COMMAND assetconv "${INPUT_PATH}" "${OUTPUT_PATH}"
            DEPENDS "${INPUT_PATH}" "${MANIFEST}" "${ASSET_FORMAT_HEADER}" assetconv
            COMMENT "Converting ${SOURCE} to ${OUTPUT}"
            VERBATIM)
    list(APPEND CONVERTED_ASSETS "${OUTPUT_PATH}")
endforeach ()

# Standalone textures: one baked KTX2 each, at their own size. For images too
# large to share a sprite sheet.
string(JSON TEXTURE_COUNT ERROR_VARIABLE NO_TEXTURES LENGTH "${MANIFEST_JSON}" textures)
if (NOT TEXTURE_COUNT)
    set(TEXTURE_COUNT 0)
endif ()
message(STATUS "Asset manifest lists ${TEXTURE_COUNT} texture(s)")

if (TEXTURE_COUNT GREATER 0)
    math(EXPR LAST_TEXTURE "${TEXTURE_COUNT} - 1")
    foreach (INDEX RANGE ${LAST_TEXTURE})
        string(JSON ENTRY GET "${MANIFEST_JSON}" textures ${INDEX})
        string(JSON SOURCE GET "${ENTRY}" source)
        string(JSON OUTPUT GET "${ENTRY}" output)

        set(INPUT_PATH "${ASSET_SOURCE_DIR}/${SOURCE}")
        set(OUTPUT_PATH "${ASSET_OUTPUT_DIR}/${OUTPUT}")
        add_custom_command(
                OUTPUT "${OUTPUT_PATH}"
                COMMAND assetconv --texture "${OUTPUT_PATH}" "${INPUT_PATH}"
                DEPENDS "${INPUT_PATH}" "${MANIFEST}" assetconv
                COMMENT "Converting ${SOURCE} to ${OUTPUT}"
                VERBATIM)
        list(APPEND CONVERTED_ASSETS "${OUTPUT_PATH}")
    endforeach ()
endif ()

# Sprite atlases. Each entry names its sprites, so the lookup name in the
# engine survives a source file being renamed or moved.
string(JSON ATLAS_COUNT ERROR_VARIABLE NO_ATLASES LENGTH "${MANIFEST_JSON}" atlases)
if (NOT ATLAS_COUNT)
    set(ATLAS_COUNT 0)
endif ()
message(STATUS "Asset manifest lists ${ATLAS_COUNT} atlas(es)")

if (ATLAS_COUNT GREATER 0)
    math(EXPR LAST_ATLAS "${ATLAS_COUNT} - 1")
    foreach (INDEX RANGE ${LAST_ATLAS})
        string(JSON ENTRY GET "${MANIFEST_JSON}" atlases ${INDEX})
        string(JSON OUTPUT GET "${ENTRY}" output)
        string(JSON SPRITES GET "${ENTRY}" sprites)
        string(JSON SPRITE_COUNT LENGTH "${SPRITES}")

        set(ATLAS_ARGS "")
        set(ATLAS_INPUTS "")
        math(EXPR LAST_SPRITE "${SPRITE_COUNT} - 1")
        foreach (SPRITE_INDEX RANGE ${LAST_SPRITE})
            string(JSON SPRITE_NAME MEMBER "${SPRITES}" ${SPRITE_INDEX})
            string(JSON SPRITE_PATH GET "${SPRITES}" "${SPRITE_NAME}")
            list(APPEND ATLAS_ARGS "${SPRITE_NAME}=${ASSET_SOURCE_DIR}/${SPRITE_PATH}")
            list(APPEND ATLAS_INPUTS "${ASSET_SOURCE_DIR}/${SPRITE_PATH}")
        endforeach ()

        set(OUTPUT_PATH "${ASSET_OUTPUT_DIR}/${OUTPUT}")
        add_custom_command(
                OUTPUT "${OUTPUT_PATH}"
                COMMAND assetconv --atlas "${OUTPUT_PATH}" ${ATLAS_ARGS}
                DEPENDS ${ATLAS_INPUTS} "${MANIFEST}" assetconv
                COMMENT "Packing ${SPRITE_COUNT} sprites into ${OUTPUT}"
                VERBATIM)
        list(APPEND CONVERTED_ASSETS "${OUTPUT_PATH}")
    endforeach ()
endif ()

# Shaders. Every stage compiles into one pack, keyed by the name the engine
# asks for. Each stage is <source>:<entry point>, source relative to assets/.
string(JSON SHADER_OUTPUT GET "${MANIFEST_JSON}" shaders output)
string(JSON SHADER_STAGES GET "${MANIFEST_JSON}" shaders stages)
string(JSON STAGE_COUNT LENGTH "${SHADER_STAGES}")
message(STATUS "Asset manifest lists ${STAGE_COUNT} shader stage(s)")

set(SHADER_ARGS "")
set(SHADER_INPUTS "")
math(EXPR LAST_STAGE "${STAGE_COUNT} - 1")
foreach (INDEX RANGE ${LAST_STAGE})
    string(JSON STAGE_NAME MEMBER "${SHADER_STAGES}" ${INDEX})
    string(JSON STAGE GET "${SHADER_STAGES}" "${STAGE_NAME}")
    list(APPEND SHADER_ARGS "${STAGE_NAME}=${ASSET_SOURCE_DIR}/${STAGE}")
    string(REGEX REPLACE ":[^:]*$" "" STAGE_SOURCE "${STAGE}")
    list(APPEND SHADER_INPUTS "${ASSET_SOURCE_DIR}/${STAGE_SOURCE}")
endforeach ()
list(REMOVE_DUPLICATES SHADER_INPUTS)

# One output for every stage, so DEPENDS has to name every file any stage
# reads, includes as well, or a shader edit stops rebuilding the pack.
file(GLOB SHADER_INCLUDES CONFIGURE_DEPENDS
        "${ASSET_SOURCE_DIR}/shaders/slang/include/*.slang")

set(OUTPUT_PATH "${ASSET_OUTPUT_DIR}/${SHADER_OUTPUT}")
add_custom_command(
        OUTPUT "${OUTPUT_PATH}"
        COMMAND assetconv --shaders "${OUTPUT_PATH}"
                "$<$<CONFIG:Release>:--no-line-directives>" ${SHADER_ARGS}
        DEPENDS ${SHADER_INPUTS} ${SHADER_INCLUDES} "${MANIFEST}" assetconv
        COMMENT "Compiling ${STAGE_COUNT} shader stages into ${SHADER_OUTPUT}"
        VERBATIM
        COMMAND_EXPAND_LISTS)
list(APPEND CONVERTED_ASSETS "${OUTPUT_PATH}")

add_custom_target(convert_assets ALL DEPENDS ${CONVERTED_ASSETS})

# Bake inputs must not ship beside what they were baked into. Every image
# under assets/ is a bake input, so one extension pattern covers them all, and
# manifest.json drives the bake and is never read at run time. Consumed by
# CopyAssets.cmake and by the install rules in game/CMakeLists.txt, both of
# which match a REGEX against the whole path.
# CMake regexes have no case-insensitive flag, and Linux does not fold the path
# case, so each letter is spelled as a two-case class.
set(BAKED_SOURCE_REGEX
        "\\.([Pp][Nn][Gg]|[Jj][Pp][Ee]?[Gg])$|manifest\\.json$")
