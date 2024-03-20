#include "imguinullmanager.hpp"
#include "platform/sdl/sdlengine.hpp"

namespace sponge::imgui {

void ImGuiNullManager::onAttach() {
    // nothing
}

void ImGuiNullManager::onDetach() {
    // nothing
}

bool ImGuiNullManager::isEventHandled() {
    return false;
}

void ImGuiNullManager::begin() {
    // nothing
}

void ImGuiNullManager::end() {
    // nothing
}

void ImGuiNullManager::processEvent(const SDL_Event* event) {
    UNUSED(event);
    // nothing
}

}  // namespace sponge::imgui
