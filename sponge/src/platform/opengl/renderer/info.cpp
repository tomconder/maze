#include "info.hpp"
#include "logging/log.hpp"
#include "platform/opengl/renderer/gl.hpp"
#include <spdlog/fmt/fmt.h>
#include <SDL.h>
#include <cassert>
#include <numeric>
#include <sstream>
#include <vector>

namespace sponge::platform::opengl::renderer {

void Info::logContextInfo() {
    assert(SDL_GL_GetCurrentContext() && "Missing OpenGL Context");

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

void Info::logGraphicsDriverInfo() {
    assert(SDL_GL_GetCurrentContext() && "Missing OpenGL Context");

    const auto numVideoDrivers = SDL_GetNumVideoDrivers();
    SPONGE_CORE_DEBUG("Video Driver Info [{}]:", numVideoDrivers);

    const std::string currentVideoDriver(SDL_GetCurrentVideoDriver());
    for (int i = 0; i < numVideoDrivers; i++) {
        const std::string videoDriver(SDL_GetVideoDriver(i));
        std::stringstream ss;
        ss << fmt::format("  #{}: {} {}", i, videoDriver,
                          currentVideoDriver == videoDriver ? "[current]" : "");
        SPONGE_CORE_DEBUG(ss.str());
    }

    const int numRenderDrivers = SDL_GetNumRenderDrivers();
    SPONGE_CORE_DEBUG("Render Driver Info [{}]:", numRenderDrivers);

    SDL_RendererInfo info;
    for (int i = 0; i < numRenderDrivers; ++i) {
        SDL_GetRenderDriverInfo(i, &info);

        bool isSoftware = (info.flags & SDL_RENDERER_SOFTWARE) != 0U;
        bool isHardware = (info.flags & SDL_RENDERER_ACCELERATED) != 0U;
        bool isVSyncEnabled = (info.flags & SDL_RENDERER_PRESENTVSYNC) != 0U;
        bool isTargetTexture = (info.flags & SDL_RENDERER_TARGETTEXTURE) != 0U;

        std::vector<std::string> v;
        v.reserve(4);

        if (isSoftware) {
            v.emplace_back("SW");
        }
        if (isHardware) {
            v.emplace_back("HW");
        }
        if (isVSyncEnabled) {
            v.emplace_back("VSync");
        }
        if (isTargetTexture) {
            v.emplace_back("TTex");
        }

        auto flags =
            v.empty()
                ? ""
                : std::accumulate(++v.begin(), v.end(), *v.begin(),
                                  [](std::string a, std::string b) {
                                      return std::move(a) + ", " + std::move(b);
                                  });

        std::stringstream ss;

        ss << fmt::format("  #{}: {:10} [{}]", i, info.name, flags.c_str());
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
