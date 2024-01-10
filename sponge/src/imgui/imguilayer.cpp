#include "imguilayer.h"
#include "event/event.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include "platform/sdl/sdlengine.h"

namespace sponge::imgui {

ImGuiLayer::ImGuiLayer() : Layer("imgui-engine") {
    // nothing
}

void ImGuiLayer::onAttach() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    auto* context = SDL_GL_GetCurrentContext();
    auto* window = SDL_GL_GetCurrentWindow();
    ImGui_ImplSDL2_InitForOpenGL(window, context);
    ImGui_ImplOpenGL3_Init();
}

void ImGuiLayer::onDetach() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiLayer::onEvent(event::Event& event) {
    const auto& io = ImGui::GetIO();
    event.handled |=
        static_cast<uint8_t>(event.isInCategory(event::EventCategoryMouse)) &
        static_cast<uint8_t>(io.WantCaptureMouse);
    event.handled |=
        static_cast<uint8_t>(event.isInCategory(event::EventCategoryKeyboard)) &
        static_cast<uint8_t>(io.WantCaptureKeyboard);
}

void ImGuiLayer::begin() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::end() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiLayer::processEvent(const SDL_Event* event) {
    ImGui_ImplSDL2_ProcessEvent(event);
}

}  // namespace sponge::imgui
