# Asset converter. Links sponge::format for the KTX2 codec and the baked
# format writers, so the converter and the engine can never disagree about
# the layout.
file(
        GLOB SOURCE_FILES
        CONFIGURE_DEPENDS
        LIST_DIRECTORIES false
        src/*.cpp)

file(
        GLOB HEADER_FILES
        CONFIGURE_DEPENDS
        LIST_DIRECTORIES false
        src/*.hpp)

add_executable(assetconv ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(assetconv PRIVATE src)

# Every asset format lives here, not in the engine: cgltf for glTF,
# tinyobjloader for OBJ, stb_image to decode source images, bc7enc for BC7
# blocks, stb_image_resize2 for the mip chain.
# bc7enc_rdo @ b9438627eef73a1157e84201b6fa6eb2ffd6d9f0
# cgltf, stb @ vendored with the engine originally, see git history
# Own target so they take the engine's options but not its warnings.
add_library(assetconv_deps OBJECT
        deps/bc7enc/bc7enc.cpp
        deps/cgltf/cgltf.cpp
        deps/stb/stb_image.cpp
        deps/tinyobjloader/tiny_obj_loader.cpp)
set_target_properties(assetconv_deps PROPERTIES CXX_STANDARD 23)
target_include_directories(assetconv_deps PUBLIC
        deps/bc7enc deps/cgltf deps/stb deps/tinyobjloader)
target_link_libraries(assetconv_deps PRIVATE sponge::options)
target_compile_options(assetconv_deps PRIVATE
        $<IF:$<CXX_COMPILER_ID:MSVC>,/W0,-w>)
target_link_libraries(assetconv PRIVATE assetconv_deps)

# stb_rect_pack packs the sprite atlas; the engine's copy serves both. It
# builds clean at /W4.
target_sources(assetconv PRIVATE
        ${CMAKE_SOURCE_DIR}/sponge/deps/stb/stb_rect_pack.cpp)
target_include_directories(assetconv PRIVATE
        ${CMAKE_SOURCE_DIR}/sponge/deps/stb)

# Mesh optimization is a bake step now, so meshoptimizer links here.
find_package(meshoptimizer CONFIG REQUIRED)
target_link_libraries(assetconv PRIVATE meshoptimizer::meshoptimizer)

target_link_libraries(assetconv PRIVATE sponge::format)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(assetconv PRIVATE fmt::fmt)

# BC7 encoding splits each image across threads.
find_package(Threads REQUIRED)
target_link_libraries(assetconv PRIVATE Threads::Threads)

# Reads assets/manifest.json.
find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(assetconv PRIVATE nlohmann_json::nlohmann_json)

# Slang compiles the shaders in-process. It loads its GLSL module and glslang
# by name at run time, so those sit beside the exe along with the DLLs it
# links against.
find_package(slang CONFIG REQUIRED)
target_link_libraries(assetconv PRIVATE slang::slang)
if (WIN32)
    add_custom_command(TARGET assetconv POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:assetconv>
                $<TARGET_FILE:slang::slang-glsl-module>
                $<TARGET_FILE:slang::slang-glslang>
                $<TARGET_FILE_DIR:assetconv>
            COMMAND_EXPAND_LISTS)
endif ()

# fmt trips C4702 (unreachable code) inside its own headers under MSVC at
# /W4 /WX. The vcpkg includes are already -external:W0, but C4702 comes from
# the optimizer, and /external governs only frontend warnings.
target_compile_options(assetconv PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/wd4702>)

set_target_properties(assetconv PROPERTIES CXX_STANDARD 23)

# Host tool: never installed with the game.
