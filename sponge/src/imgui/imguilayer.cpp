#include "imguilayer.h"
#include "event/event.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"

namespace sponge::imgui {

ImGuiLayer::ImGuiLayer() : Layer("imgui") {
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
    ImGuiIO& io = ImGui::GetIO();
    event.handled |=
        static_cast<uint8_t>(event.isInCategory(event::EventCategoryMouse)) &
        static_cast<uint8_t>(io.WantCaptureMouse);
    event.handled |=
        static_cast<uint8_t>(event.isInCategory(event::EventCategoryKeyboard)) &
        static_cast<uint8_t>(io.WantCaptureKeyboard);

    SPONGE_INFO("Event[{}] :: Mouse[{}] Key[{}]", event.handled,
                event.isInCategory(event::EventCategoryMouse),
                event.isInCategory(event::EventCategoryKeyboard));
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

void ImGuiLayer::render() {
    static float f = 0.0F;
    static int counter = 0;
    bool show_another_window = false;

    ImGuiIO& io = ImGui::GetIO();

    ImGui::Begin("Hello, world!");

    ImGui::Text("This is some useful text.");
    ImGui::Checkbox("Another Window", &show_another_window);
    ImGui::SliderFloat("float", &f, 0.F, 1.F);

    if (ImGui::Button("Button")) {
        counter++;
    }
    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);

    ImGui::Text("Dispatch mouse = %d",
                static_cast<uint8_t>(io.WantCaptureMouse));
    ImGui::Text("Dispatch keyboard = %d",
                static_cast<uint8_t>(io.WantCaptureKeyboard));

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0F / io.Framerate, io.Framerate);

    ImGui::End();
}

}  // namespace sponge::imgui
