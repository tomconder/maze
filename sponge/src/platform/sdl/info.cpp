#include "info.hpp"
#include "core/log.hpp"

namespace sponge::platform::sdl {

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

}  // namespace sponge::platform::sdl
