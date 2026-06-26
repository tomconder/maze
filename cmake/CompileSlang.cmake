find_program(SLANGC_EXECUTABLE slangc
    REQUIRED
    HINTS
        "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows/tools/shader-slang"
        "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-linux/tools/shader-slang"
        "${CMAKE_SOURCE_DIR}/vcpkg_installed/arm64-osx/tools/shader-slang"
)
message(STATUS "Found slangc: ${SLANGC_EXECUTABLE}")

set(SLANG_SOURCE_DIR "${CMAKE_SOURCE_DIR}/assets/shaders/slang")
set(SLANG_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders/glsl")
file(MAKE_DIRECTORY "${SLANG_OUTPUT_DIR}")

# Usage: compile_slang_shader(<slang-file> <entry-point> <output-glsl-name>)
# Appends the output path to SLANG_COMPILED_OUTPUTS (parent scope).
function(compile_slang_shader SLANG_FILE ENTRY_POINT OUTPUT_NAME)
    set(INPUT  "${SLANG_SOURCE_DIR}/${SLANG_FILE}")
    set(OUTPUT "${SLANG_OUTPUT_DIR}/${OUTPUT_NAME}")
    file(GLOB SLANG_INCLUDES "${SLANG_SOURCE_DIR}/include/*.slang")
    add_custom_command(
        OUTPUT  "${OUTPUT}"
        COMMAND "${SLANGC_EXECUTABLE}"
                "${INPUT}"
                -entry "${ENTRY_POINT}"
                -target glsl
                -profile glsl_450
                -matrix-layout-column-major
                "$<$<CONFIG:Release>:-line-directive-mode;none>"
                -o "${OUTPUT}"
        DEPENDS "${INPUT}" ${SLANG_INCLUDES}
        COMMENT "Compiling Slang file ${SLANG_FILE} [${ENTRY_POINT}] to target ${OUTPUT_NAME}"
        VERBATIM
        COMMAND_EXPAND_LISTS
    )
    set(SLANG_COMPILED_OUTPUTS "${SLANG_COMPILED_OUTPUTS};${OUTPUT}" PARENT_SCOPE)
endfunction()

set(SLANG_COMPILED_OUTPUTS "")

# Usage: compile_slang_compute_shader(<slang-file> <entry-point> <output-name>)
function(compile_slang_compute_shader SLANG_FILE ENTRY_POINT OUTPUT_NAME)
    set(INPUT  "${SLANG_SOURCE_DIR}/${SLANG_FILE}")
    set(OUTPUT "${SLANG_OUTPUT_DIR}/${OUTPUT_NAME}")
    file(GLOB SLANG_INCLUDES "${SLANG_SOURCE_DIR}/include/*.slang")
    add_custom_command(
        OUTPUT  "${OUTPUT}"
        COMMAND "${SLANGC_EXECUTABLE}"
                "${INPUT}"
                -entry "${ENTRY_POINT}"
                -stage compute
                -target glsl
                -profile glsl_450
                -matrix-layout-column-major
                "$<$<CONFIG:Release>:-line-directive-mode;none>"
                -o "${OUTPUT}"
        DEPENDS "${INPUT}" ${SLANG_INCLUDES}
        COMMENT "Compiling Slang compute shader ${SLANG_FILE} [${ENTRY_POINT}] to ${OUTPUT_NAME}"
        VERBATIM
        COMMAND_EXPAND_LISTS
    )
    set(SLANG_COMPILED_OUTPUTS "${SLANG_COMPILED_OUTPUTS};${OUTPUT}" PARENT_SCOPE)
endfunction()

# screenquad vertex (shared by blur, bloom, and fxaa passes)
compile_slang_shader(fxaa.slang                vertMain   screenquad.vert.glsl)

# PBR
compile_slang_shader(pbr.slang                 fragMain   pbr.frag.glsl)
compile_slang_shader(pbr.slang                 vertMain   pbr.vert.glsl)

# Shadow map passes
compile_slang_shader(shadowmap.slang           fragMain     shadowmap.frag.glsl)
compile_slang_shader(shadowmap.slang           fragMainEVSM shadowmap_evsm.frag.glsl)
compile_slang_shader(shadowmap.slang           vertMain     shadowmap.vert.glsl)

# Shadow blur (Dual Kawase, EVSM RG channels)
compile_slang_shader(blur.slang                downFragMain      blur.frag.glsl)
compile_slang_shader(blur.slang                upFragMain        blur_up.frag.glsl)

# Bloom pipeline
compile_slang_shader(bloom.slang               compositeFragMain bloom_composite.frag.glsl)
compile_slang_shader(bloom.slang               downFragMain      bloom_down.frag.glsl)
compile_slang_shader(bloom.slang               extractFragMain   bloom_extract.frag.glsl)
compile_slang_shader(bloom.slang               upFragMain        bloom_up.frag.glsl)

# FXAA
compile_slang_shader(fxaa.slang                fragMain   fxaa.frag.glsl)

# 2D / UI
compile_slang_shader(rectangle.slang           fragMain      quad.frag.glsl)
compile_slang_shader(rectangle.slang           vertMain      quad.vert.glsl)
compile_slang_shader(sprite.slang              fragMain      sprite.frag.glsl)
compile_slang_shader(sprite.slang              textFragMain  text.frag.glsl)
compile_slang_shader(sprite.slang              vertMain      sprite.vert.glsl)

# Light cube
compile_slang_shader(cube.slang                fragMain   cube.frag.glsl)
compile_slang_shader(cube.slang                vertMain   cube.vert.glsl)

# Depth prepass (depth-only)
compile_slang_shader(depthprepass.slang vertMain depthprepass.vert.glsl)
compile_slang_shader(depthprepass.slang fragMain depthprepass.frag.glsl)

# Cluster light assignment compute shader
compile_slang_compute_shader(cluster_assign.slang csMain cluster_assign.comp.glsl)

add_custom_target(compile_shaders ALL
    DEPENDS ${SLANG_COMPILED_OUTPUTS}
)
