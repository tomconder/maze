#pragma once

#include "imguimanager.hpp"

namespace sponge::imgui {

class ImGuiNullManager final : public ImGuiManager {
   public:
    static void onAttach();
    static void onDetach();
    static bool isEventHandled();

    static void begin();
    static void end();
    static void processEvent(const SDL_Event* event);
};

}  // namespace sponge::imgui
