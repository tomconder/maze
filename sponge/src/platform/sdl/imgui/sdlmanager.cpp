#include "sdlmanager.hpp"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include "platform/sdl/engine.hpp"

namespace sponge::platform::sdl::imgui {

void SDLManager::onAttachImpl() {
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

void SDLManager::onDetachImpl() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

bool SDLManager::isEventHandledImpl() {
    const auto& io = ImGui::GetIO();
    return io.WantCaptureMouse || io.WantCaptureKeyboard;
}

void SDLManager::beginImpl() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void SDLManager::endImpl() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void SDLManager::processEventImpl(const SDL_Event* event) {
    ImGui_ImplSDL2_ProcessEvent(event);
}

}  // namespace sponge::platform::sdl::imgui
