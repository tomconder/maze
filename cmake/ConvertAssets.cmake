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

add_custom_target(convert_assets ALL DEPENDS ${CONVERTED_ASSETS})
