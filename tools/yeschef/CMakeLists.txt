# Builds yeschef with cargo. The asset bake still runs assetconv.
find_program(CARGO cargo REQUIRED)

set(YESCHEF_TARGET_DIR "${CMAKE_CURRENT_BINARY_DIR}/cargo")
set(YESCHEF_EXE "${YESCHEF_TARGET_DIR}/release/yeschef${CMAKE_EXECUTABLE_SUFFIX}")

add_custom_target(yeschef ALL
        COMMAND ${CARGO} build --release --locked
        --manifest-path "${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml"
        --target-dir "${YESCHEF_TARGET_DIR}"
        # cargo reads .cargo/config.toml from the working directory.
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        BYPRODUCTS "${YESCHEF_EXE}"
        COMMENT "Building yeschef"
        VERBATIM)
