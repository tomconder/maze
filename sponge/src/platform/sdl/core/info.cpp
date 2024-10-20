#include "info.hpp"
#include "logging/log.hpp"
#include <SDL.h>
#include <numeric>
#include <sstream>
#include <vector>

namespace sponge::platform::sdl::core {

void Info::logVersion() {
    SDL_version compiled;
    SDL_version linked;

    SDL_VERSION(&compiled)

    const std::string revision = SDL_GetRevision();
    std::stringstream ss;
    if (!revision.empty()) {
        ss << "(" << revision << ")";
    }

    SPONGE_CORE_DEBUG(
        "SDL Version [Compiled]: {}.{}.{} {}", static_cast<int>(compiled.major),
        static_cast<int>(compiled.minor), static_cast<int>(compiled.patch),
        !revision.empty() ? ss.str() : "");

    SDL_GetVersion(&linked);

    SPONGE_CORE_DEBUG(
        "SDL Version [Runtime] : {}.{}.{}", static_cast<int>(linked.major),
        static_cast<int>(linked.minor), static_cast<int>(linked.patch));
}

void Info::logGraphicsDriverInfo() {
    const auto numVideoDrivers = SDL_GetNumVideoDrivers();
    SPONGE_CORE_DEBUG("SDL Video Driver Info [{}]:", numVideoDrivers);

    const std::string currentVideoDriver(SDL_GetCurrentVideoDriver());
    for (int i = 0; i < numVideoDrivers; i++) {
        const std::string videoDriver(SDL_GetVideoDriver(i));
        std::stringstream ss;
        ss << fmt::format("  #{}: {} {}", i, videoDriver,
                          currentVideoDriver == videoDriver ? "[current]" : "");
        SPONGE_CORE_DEBUG(ss.str());
    }

    const int numRenderDrivers = SDL_GetNumRenderDrivers();
    SPONGE_CORE_DEBUG("SDL Render Driver Info [{}]:", numRenderDrivers);

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

}  // namespace sponge::platform::sdl::core
