# Asset converter. Links sponge for the importers, the KTX2 codec and the
# baked format writer, so the converter and the engine can never disagree
# about the layout.
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
set(VENDORED_SOURCES
        deps/bc7enc/bc7enc.cpp
        deps/cgltf/cgltf.cpp
        deps/stb/stb_image.cpp
        deps/tinyobjloader/tiny_obj_loader.cpp)
target_sources(assetconv PRIVATE ${VENDORED_SOURCES})
target_include_directories(assetconv PRIVATE
        deps/bc7enc deps/cgltf deps/stb deps/tinyobjloader)
set_source_files_properties(${VENDORED_SOURCES} PROPERTIES COMPILE_OPTIONS
        "$<IF:$<CXX_COMPILER_ID:MSVC>,/W0,-w>")

# Mesh optimization is a bake step now, so meshoptimizer links here.
find_package(meshoptimizer CONFIG REQUIRED)
target_link_libraries(assetconv PRIVATE meshoptimizer::meshoptimizer)

target_link_libraries(assetconv PRIVATE sponge::sponge)

# fmt trips C4702 (unreachable code) inside its own headers under MSVC at
# /W4 /WX. The vcpkg includes are already -external:W0, but C4702 comes from
# the optimizer, and /external governs only frontend warnings.
target_compile_options(assetconv PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/wd4702>)

set_target_properties(assetconv PROPERTIES CXX_STANDARD 23)

# Host tool: never installed with the game.
