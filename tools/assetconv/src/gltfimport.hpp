#pragma once

#include "scene/modeldata.hpp"

#include <string>

// glTF/GLB importer. Converter-only: the engine loads baked models, so
// cgltf and stb_image never reach the runtime.
namespace assetconv::gltf {

sponge::scene::ModelData parse(const std::string& path);

}  // namespace assetconv::gltf
