#pragma once

#include "shaderpack.hpp"

#include <optional>
#include <string>
#include <vector>

namespace assetconv {

struct ShaderEntry {
    std::string name;        // key the engine looks the stage up by
    std::string path;        // .slang source on disk
    std::string entryPoint;  // function tagged [shader("...")]
};

// Compiles every entry to GLSL 450 with column-major matrices. Returns
// nothing, after printing Slang's diagnostics, if any entry fails.
std::optional<sponge::scene::shaderpack::Sources>
    compileShaders(const std::vector<ShaderEntry>& entries,
                   bool                            lineDirectives);

}  // namespace assetconv
