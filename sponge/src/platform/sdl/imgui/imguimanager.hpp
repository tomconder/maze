#pragma once

#include <SDL.h>

namespace sponge::platform::sdl::imgui {

template <class T>
struct ImGuiManager {
   public:
    void onAttach() {
        (static_cast<T*>(this))->onAttachImpl();
    }

    void onDetach() {
        (static_cast<T*>(this))->onDetachImpl();
    }

    bool isEventHandled() {
        return (static_cast<T*>(this))->isEventHandledImpl();
    }

    void begin() {
        (static_cast<T*>(this))->beginImpl();
    }

    void end() {
        (static_cast<T*>(this))->endImpl();
    }

    void processEvent(const SDL_Event* event) {
        (static_cast<T*>(this))->processEventImpl(event);
    }
};

}  // namespace sponge::platform::sdl::imgui
