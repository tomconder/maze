#include "info.hpp"
#include "logging/log.hpp"
#include "platform/opengl/renderer/gl.hpp"
#include <spdlog/fmt/fmt.h>
#include <sstream>

namespace sponge::platform::opengl::renderer {

void Info::logContextInfo() {
    SPONGE_CORE_INFO("OpenGL Info:");

    std::stringstream ss;
    ss << fmt::format("  {:14} {}", "Version:",
                      reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    SPONGE_CORE_INFO(ss.str());

    ss.str("");
    ss << fmt::format("  {:14} {}", "GLSL:",
                      reinterpret_cast<const char*>(
                          glGetString(GL_SHADING_LANGUAGE_VERSION)));
    SPONGE_CORE_INFO(ss.str());

    ss.str("");
    ss << fmt::format("  {:14} {}", "Renderer:",
                      reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    SPONGE_CORE_INFO(ss.str());

    ss.str("");
    ss << fmt::format("  {:14} {}", "Vendor:",
                      reinterpret_cast<const char*>(glGetString(GL_VENDOR)));

    SPONGE_CORE_INFO(ss.str());

    int32_t extensions;
    glGetIntegerv(GL_NUM_EXTENSIONS, &extensions);
    ss.str("");
    ss << fmt::format("  {:14} {}", "Extensions:", extensions);
    SPONGE_CORE_DEBUG(ss.str());

    for (int i = 0; i < extensions / 3; i++) {
        ss.str("");
        ss << "   ";
        for (int j = 0; j < 3; j++) {
            ss << fmt::format(" {:48}",
                              reinterpret_cast<const char*>(
                                  glGetStringi(GL_EXTENSIONS, (i * 3) + j)));
        }
        SPONGE_CORE_DEBUG(ss.str());
    }
}

void Info::logStaticInfo() {
#ifdef GL_GLEXT_VERSION
    SPONGE_CORE_DEBUG("OpenGL GLEXT version: {}", GL_GLEXT_VERSION);
#endif
}

void Info::logVersion() {
    auto minorVersion = 0;
    auto majorVersion = 0;
    glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
    glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);

    SPONGE_CORE_INFO("Created OpenGL context: {}.{}", majorVersion,
                     minorVersion);
}

}  // namespace sponge::platform::opengl::renderer
