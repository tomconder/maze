#include "platform/opengl/debug/diagnostics.hpp"

#include "logging/log.hpp"
#include "platform/opengl/renderer/gl.hpp"

#include <fmt/format.h>

#include <sstream>

namespace sponge::platform::opengl::debug {
void Diagnostics::log() {
    auto minorVersion = 0;
    auto majorVersion = 0;
    glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
    glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);

    SPONGE_GL_INFO("Detected OpenGL version {}.{}", majorVersion, minorVersion);

#ifdef GL_GLEXT_VERSION
    SPONGE_GL_DEBUG("OpenGL GLEXT version: {}", GL_GLEXT_VERSION);
#endif

    const char* glslVersionPtr =
        reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
    const std::string glslVersion = glslVersionPtr ? glslVersionPtr : "Unknown";
    const char*       vendorPtr =
        reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const std::string vendor = vendorPtr ? vendorPtr : "Unknown";
    const char*       rendererPtr =
        reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    const std::string renderer = rendererPtr ? rendererPtr : "Unknown";
    const char*       versionPtr =
        reinterpret_cast<const char*>(glGetString(GL_VERSION));
    const std::string version = versionPtr ? versionPtr : "Unknown";

    SPONGE_GL_INFO("Detected GLSL version {}", glslVersion);
    SPONGE_GL_INFO("OpenGL graphics engine:");
    SPONGE_GL_INFO("  {:12} {}", "Vendor:", vendor);
    SPONGE_GL_INFO("  {:12} {}", "Renderer:", renderer);
    SPONGE_GL_INFO("  {:12} {}", "Version:", version);

    int32_t extensions = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &extensions);
    SPONGE_GL_INFO("  {:12} {}", "Extensions:", extensions);

    std::stringstream ss;
    for (int i = 0; i < extensions / 3; i++) {
        ss.str("");
        ss << "   ";
        for (int j = 0; j < 3; j++) {
            const char* extPtr = reinterpret_cast<const char*>(
                glGetStringi(GL_EXTENSIONS, (i * 3) + j));
            const std::string ext = extPtr ? extPtr : "";
            ss << fmt::format(" {:49}", ext);
        }
        SPONGE_GL_DEBUG(ss.str());
    }
}
}  // namespace sponge::platform::opengl::debug
