#pragma once

#include "core/application.hpp"
#include "event/event.hpp"
#include "input/keyboard.hpp"
#include "layer/layer.hpp"
#include "layer/layerstack.hpp"
#include "logging/log.hpp"
#include "platform/opengl/context.hpp"
#include "platform/opengl/rendererapi.hpp"
#include "platform/sdl/imgui/imguimanager.hpp"
#include "platform/sdl/window.hpp"
#include <SDL.h>
#include <cstdint>
#include <memory>
#include <string>

namespace sponge::platform::sdl {

using logging::LogItem;

class Application : public sponge::Application {
   public:
    Application();

    bool construct(std::string_view name, uint32_t width, uint32_t height);

    bool start() override;

    bool iterateLoop() override;

    void shutdown() override;

    bool onUserCreate() override;

    bool onUserUpdate(double elapsedTime) override;

    bool onUserDestroy() override;

    void onEvent(event::Event& event) override;

    void onImGuiRender();

    void adjustAspectRatio(uint32_t eventW, uint32_t eventH);

    void pushOverlay(const std::shared_ptr<layer::Layer>& layer) const;

    void pushLayer(const std::shared_ptr<layer::Layer>& layer) const;

    void popLayer(const std::shared_ptr<layer::Layer>& layer) const;

    void popOverlay(const std::shared_ptr<layer::Layer>& layer) const;

    void toggleFullscreen() const;

    bool isFullscreen() const {
        const auto flags = SDL_GetWindowFlags(
            static_cast<SDL_Window*>(sdlWindow->getNativeWindow()));
        return (flags & SDL_WINDOW_FULLSCREEN) != 0;
    }

    layer::LayerStack* getLayerStack() const {
        return layerStack;
    }

    uint32_t getHeight() const {
        return h;
    }

    uint32_t getWidth() const {
        return w;
    }

    uint32_t getWindowHeight() const {
        int32_t w;
        int32_t h;
        SDL_GetWindowSize(
            static_cast<SDL_Window*>(sdlWindow->getNativeWindow()), &w, &h);
        return static_cast<uint32_t>(h);
    }

    uint32_t getWindowWidth() const {
        int32_t w;
        int32_t h;
        SDL_GetWindowSize(
            static_cast<SDL_Window*>(sdlWindow->getNativeWindow()), &w, &h);
        return static_cast<uint32_t>(w);
    }

    static bool hasVerticalSync() {
        return SDL_GL_GetSwapInterval() != 0;
    }

    static void setVerticalSync(bool value) {
        SDL_GL_SetSwapInterval(value ? 1 : 0);
    }

    std::vector<LogItem>& getMessages() const {
        return *messages;
    }

    void addMessage(const LogItem& item) const {
        messages->push_back(item);
    }

    void clearMessages() const {
        messages->clear();
    }

    void setMouseVisible(bool value) const;

    static Application& get() {
        return *instance;
    }

   private:
    std::string appName = "undefined";
    std::unique_ptr<opengl::Context> graphics;
    std::unique_ptr<opengl::RendererAPI> renderer;
    std::unique_ptr<Window> sdlWindow;

    std::unique_ptr<std::vector<LogItem>> messages;

    uint32_t offsetx = 0;
    uint32_t offsety = 0;
    uint32_t w = 0;
    uint32_t h = 0;

    layer::LayerStack* layerStack;
    input::Keyboard* keyboard;

    void processEvent(const SDL_Event& event, double elapsedTime);

    static Application* instance;
};

}  // namespace sponge::platform::sdl
