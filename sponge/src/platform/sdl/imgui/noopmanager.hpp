#pragma once

#include "core/base.hpp"
#include "imguimanager.hpp"

namespace sponge::platform::sdl::imgui {

class NoopManager : public ImGuiManager<NoopManager> {
   public:
    void onAttachImpl() { /* nothing */ }

    void onDetachImpl() { /* nothing */ }

    bool isEventHandledImpl() {
        return false;
    }

    void beginImpl() { /* nothing */ }

    void endImpl() { /* nothing */ }

    void processEventImpl(const SDL_Event* event) {
        UNUSED(event);
    }
};
}  // namespace sponge::platform::sdl::imgui
