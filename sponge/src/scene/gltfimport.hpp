#pragma once

#include "scene/modeldata.hpp"

#include <cstddef>
#include <functional>
#include <string>

// glTF/GLB importer. Produces CPU-side data with no GL or OS calls, so it
// runs in tools/assetconv as well as in the engine.
//
// The engine only needs this while models are still loaded from .glb at
// runtime. Once every model is baked to .spnga it moves to the converter
// and cgltf and stb_image leave the game.
namespace sponge::scene::gltf {

// onMeshParsed, when set, fires once per mesh appended to the result.
ModelData parse(const std::string&           path,
                const std::function<void()>& onMeshParsed = {});

// Structural mesh count without decoding vertex or image data, for
// progress-bar sizing.
std::size_t countMeshes(const std::string& path);

}  // namespace sponge::scene::gltf
