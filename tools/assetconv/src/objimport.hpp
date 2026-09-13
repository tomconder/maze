#pragma once

#include "scene/modeldata.hpp"

#include <string>

// Wavefront OBJ importer. Converter-only: the engine loads baked models, so
// tinyobjloader never reaches the runtime.
namespace assetconv::obj {

// Returns an empty ModelData on any failure, having said why on stderr.
sponge::scene::ModelData parse(const std::string& path);

}  // namespace assetconv::obj
