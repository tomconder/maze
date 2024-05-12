#pragma once

#include "imguimanager.hpp"

namespace sponge::platform::sdl::imgui {

class SDLManager : public ImGuiManager<SDLManager> {
   public:
    void onAttachImpl();

    void onDetachImpl();

    bool isEventHandledImpl();

    void beginImpl();

    void endImpl();

    void processEventImpl(const SDL_Event* event);
};
}  // namespace sponge::platform::sdl::imgui
