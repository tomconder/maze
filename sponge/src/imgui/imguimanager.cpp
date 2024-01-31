#include "imguimanager.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include "platform/sdl/sdlengine.h"

namespace sponge::imgui {

void ImGuiManager::onAttach() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    io.IniFilename = nullptr;

    auto* context = SDL_GL_GetCurrentContext();
    auto* window = SDL_GL_GetCurrentWindow();
    ImGui_ImplSDL2_InitForOpenGL(window, context);
    ImGui_ImplOpenGL3_Init();
}

void ImGuiManager::onDetach() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

bool ImGuiManager::isEventHandled() {
    const auto& io = ImGui::GetIO();
    return io.WantCaptureMouse || io.WantCaptureKeyboard;
}

void ImGuiManager::begin() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void ImGuiManager::end() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiManager::processEvent(const SDL_Event* event) {
    ImGui_ImplSDL2_ProcessEvent(event);
}

}  // namespace sponge::imgui
