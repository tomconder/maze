# Bakes the assets listed in assets/manifest.json to the engine format.
# Nothing is converted by globbing: the manifest is the only list.
#
# Mirrors CompileSlang.cmake — one custom command per asset, collected into a
# single always-checked target.

set(MANIFEST "${CMAKE_SOURCE_DIR}/assets/manifest.json")
set(ASSET_SOURCE_DIR "${CMAKE_SOURCE_DIR}/assets")
set(ASSET_OUTPUT_DIR "${CMAKE_BINARY_DIR}/assets")
set(ASSET_FORMAT_HEADER "${CMAKE_SOURCE_DIR}/sponge/src/scene/assetformat.hpp")

file(READ "${MANIFEST}" MANIFEST_JSON)
string(JSON MODEL_COUNT LENGTH "${MANIFEST_JSON}" models)
message(STATUS "Asset manifest lists ${MODEL_COUNT} model(s)")

set(CONVERTED_ASSETS "")

# Every source file the manifest consumes. These are inputs to the build, so
# the deploy must not also ship them next to what they were baked into.
set(BAKED_SOURCES "")

math(EXPR LAST_MODEL "${MODEL_COUNT} - 1")
foreach (INDEX RANGE ${LAST_MODEL})
    string(JSON ENTRY GET "${MANIFEST_JSON}" models ${INDEX})
    string(JSON SOURCE GET "${ENTRY}" source)
    string(JSON OUTPUT GET "${ENTRY}" output)

    set(INPUT_PATH "${ASSET_SOURCE_DIR}/${SOURCE}")
    set(OUTPUT_PATH "${ASSET_OUTPUT_DIR}/${OUTPUT}")
    list(APPEND BAKED_SOURCES "${SOURCE}")

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
        list(APPEND BAKED_SOURCES "${SOURCE}")
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
            list(APPEND BAKED_SOURCES "${SPRITE_PATH}")
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

add_custom_target(convert_assets ALL DEPENDS ${CONVERTED_ASSETS})

# Escapes a path for the alternation below. Every letter becomes a two-case
# class: on Windows, file(COPY) and install(DIRECTORY) match their REGEX
# against a lowercased path, so an uppercase letter in the pattern never
# matches and the file silently ships anyway. Lowercasing the pattern
# instead would then fail on case-sensitive filesystems.
function(baked_source_pattern INPUT OUTPUT)
    set(RESULT "")
    string(LENGTH "${INPUT}" LENGTH)
    if (LENGTH GREATER 0)
        math(EXPR LAST_CHAR "${LENGTH} - 1")
        foreach (INDEX RANGE ${LAST_CHAR})
            string(SUBSTRING "${INPUT}" ${INDEX} 1 CHAR)
            if (CHAR MATCHES "[A-Za-z]")
                string(TOUPPER "${CHAR}" UPPER)
                string(TOLOWER "${CHAR}" LOWER)
                string(APPEND RESULT "[${UPPER}${LOWER}]")
            elseif (CHAR STREQUAL ".")
                string(APPEND RESULT "\\.")
            else ()
                string(APPEND RESULT "${CHAR}")
            endif ()
        endforeach ()
    endif ()
    set(${OUTPUT} "${RESULT}" PARENT_SCOPE)
endfunction()

# One anchored alternation for file(COPY) and install(DIRECTORY), which match
# a REGEX against the whole path. Only the manifest's own inputs are listed:
# excluding whole directories would drop the licence files that sit beside
# the art they cover.
set(BAKED_SOURCE_REGEX "")
foreach (SOURCE IN LISTS BAKED_SOURCES)
    baked_source_pattern("${SOURCE}" ESCAPED)
    if (BAKED_SOURCE_REGEX)
        string(APPEND BAKED_SOURCE_REGEX "|")
    endif ()
    string(APPEND BAKED_SOURCE_REGEX "${ESCAPED}")
endforeach ()
baked_source_pattern("manifest.json" MANIFEST_PATTERN)
# manifest.json drives the bake and is never read at run time.
set(BAKED_SOURCE_REGEX "(${BAKED_SOURCE_REGEX}|${MANIFEST_PATTERN})$")

list(LENGTH BAKED_SOURCES BAKED_SOURCE_COUNT)
message(STATUS "Excluding ${BAKED_SOURCE_COUNT} baked source(s) from the deploy")
