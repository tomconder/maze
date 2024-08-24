#pragma once

#include "imguimanager.hpp"

namespace sponge::platform::sdl::imgui {

class SDLManager : public ImGuiManager<SDLManager> {
   public:
    static void onAttachImpl();

    static void onDetachImpl();

    static bool isEventHandledImpl();

    static void beginImpl();

    static void endImpl();

    static void processEventImpl(const SDL_Event* event);
};
}  // namespace sponge::platform::sdl::imgui
