// sponge/src/platform/opengl/renderer/shaderutils.hpp
#pragma once

#include "core/assetfiles.hpp"
#include "logging/log.hpp"
#include "scene/format/shaderpack.hpp"

#include <string>
#include <string_view>

namespace sponge::platform::opengl::renderer {

// GLSL for one baked stage, by its manifest name. The pack loads on first use
// and stays for the life of the process; it is about 50 KB.
inline std::string loadGlslSource(const std::string_view name) {
    static const auto sources = [] {
        const std::string          packName = "shaders/shaders.spnga";
        std::string                error;
        const auto                 bytes = core::assets::read(packName, error);
        scene::shaderpack::Sources result;
        if (error.empty()) {
            result = scene::shaderpack::read(bytes, packName, error);
        }
        if (!error.empty()) {
            SPONGE_GL_ERROR("{}", error);
        }
        return result;
    }();

    if (const auto it = sources.find(std::string{ name });
        it != sources.end()) {
        return it->second;
    }
    SPONGE_GL_ERROR("Shader pack has no stage named {}", name);
    return {};
}

}  // namespace sponge::platform::opengl::renderer
