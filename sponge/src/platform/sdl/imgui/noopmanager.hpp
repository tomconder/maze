#pragma once

#include "core/base.hpp"
#include "imguimanager.hpp"

namespace sponge::platform::sdl::imgui {

class NoopManager : public ImGuiManager<NoopManager> {
   public:
    static void onAttachImpl() { /* nothing */ }

    static void onDetachImpl() { /* nothing */ }

    static bool isEventHandledImpl() {
        return false;
    }

    static void beginImpl() { /* nothing */ }

    static void endImpl() { /* nothing */ }

    static void processEventImpl(const SDL_Event* event) {
        UNUSED(event);
    }
};
}  // namespace sponge::platform::sdl::imgui
