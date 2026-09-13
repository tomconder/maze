// sponge/src/platform/opengl/renderer/shaderutils.hpp
#pragma once

#include "core/file.hpp"
#include "logging/log.hpp"
#include "scene/shaderpack.hpp"

#include <string>
#include <string_view>

namespace sponge::platform::opengl::renderer {

// GLSL for one baked stage, by its manifest name. The pack loads on first use
// and stays for the life of the process; it is about 50 KB.
inline std::string loadGlslSource(const std::string_view name) {
    static const auto sources = scene::shaderpack::read(
        core::File::getResourceDir() + "/shaders/shaders.spnga");

    if (const auto it = sources.find(std::string{ name });
        it != sources.end()) {
        return it->second;
    }
    SPONGE_GL_ERROR("Shader pack has no stage named {}", name);
    return {};
}

}  // namespace sponge::platform::opengl::renderer
