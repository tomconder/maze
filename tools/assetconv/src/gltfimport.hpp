#pragma once

#include "scene/modeldata.hpp"

#include <cstddef>
#include <functional>
#include <string>

// glTF/GLB importer. Converter-only: the engine loads baked models, so
// cgltf and stb_image never reach the runtime.
namespace assetconv::gltf {

// onMeshParsed, when set, fires once per mesh appended to the result.
sponge::scene::ModelData parse(const std::string&           path,
                               const std::function<void()>& onMeshParsed = {});

// Structural mesh count without decoding vertex or image data, for
// progress-bar sizing.
std::size_t countMeshes(const std::string& path);

}  // namespace assetconv::gltf
