// sponge/src/platform/opengl/renderer/shaderutils.hpp
#pragma once

#include "logging/log.hpp"

#include <fstream>
#include <string>

namespace sponge::platform::opengl::renderer {

inline std::string loadGlslSource(const std::string& path) {
    std::string code;
    if (std::ifstream file(path, std::ios::in | std::ios::binary);
        file.good()) {
        file.seekg(0, std::ios::end);
        const auto size = file.tellg();
        if (size <= 0) {
            SPONGE_GL_ERROR("Unable to determine size of file: {}", path);
            return code;
        }
        file.seekg(0, std::ios::beg);
        code.resize(size);
        file.read(code.data(), size);
    } else {
        SPONGE_GL_ERROR("Unable to open shader file: {}", path);
    }
    return code;
}

}  // namespace sponge::platform::opengl::renderer
