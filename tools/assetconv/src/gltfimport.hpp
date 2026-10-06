#pragma once

#include "modeldata.hpp"

#include <filesystem>
#include <string>
#include <vector>

// glTF/GLB importer. Converter-only: the engine loads baked models, so
// cgltf and stb_image never reach the runtime.
namespace assetconv::gltf {

sponge::scene::ModelData parse(const std::string& path);

// The files `path` reads besides itself: external buffers and images. Empty
// for a GLB, for data URIs, and when the file does not parse.
std::vector<std::filesystem::path> dependencies(const std::string& path);

}  // namespace assetconv::gltf
