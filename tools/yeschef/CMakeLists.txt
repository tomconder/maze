# Builds yeschef, the asset converter, with cargo. cmake/ConvertAssets.cmake
# runs it through YESCHEF_COMMAND.
find_program(CARGO cargo REQUIRED)

# yeschef links Slang through the shader-slang crate. The vcpkg install holds
# the headers, the import library and the DLLs, so vcpkg.json is the one place
# that pins the Slang version.
set(SLANG_PREFIX "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")

set(YESCHEF_TARGET_DIR "${CMAKE_CURRENT_BINARY_DIR}/cargo")
set(YESCHEF_EXE "${YESCHEF_TARGET_DIR}/release/yeschef${CMAKE_EXECUTABLE_SUFFIX}")

add_custom_target(yeschef ALL
        COMMAND ${CMAKE_COMMAND} -E env "SLANG_DIR=${SLANG_PREFIX}"
        ${CARGO} build --release --locked
        --manifest-path "${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml"
        --target-dir "${YESCHEF_TARGET_DIR}"
        # cargo reads .cargo/config.toml from the working directory.
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        BYPRODUCTS "${YESCHEF_EXE}"
        COMMENT "Building yeschef"
        USES_TERMINAL
        VERBATIM)

# Slang loads its GLSL module and glslang by name at run time, from the folder
# of the Slang library. Windows finds the DLLs beside the exe, and the other
# platforms through the library path.
if (WIN32)
    add_custom_command(TARGET yeschef POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SLANG_PREFIX}/bin/slang.dll"
            "${SLANG_PREFIX}/bin/slang-compiler.dll"
            "${SLANG_PREFIX}/bin/slang-glsl-module.dll"
            "${SLANG_PREFIX}/bin/slang-glslang.dll"
            "${YESCHEF_TARGET_DIR}/release"
            VERBATIM)
    set(YESCHEF_COMMAND "${YESCHEF_EXE}" PARENT_SCOPE)
else ()
    if (APPLE)
        set(SLANG_LIBRARY_PATH "DYLD_LIBRARY_PATH")
    else ()
        set(SLANG_LIBRARY_PATH "LD_LIBRARY_PATH")
    endif ()
    set(YESCHEF_COMMAND ${CMAKE_COMMAND} -E env
            "${SLANG_LIBRARY_PATH}=${SLANG_PREFIX}/lib" "${YESCHEF_EXE}"
            PARENT_SCOPE)
endif ()
