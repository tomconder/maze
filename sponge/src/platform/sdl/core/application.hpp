#pragma once

#include "core/application.hpp"
#include "event/event.hpp"
#include "layer/layer.hpp"
#include "layer/layerstack.hpp"
#include "logging/log.hpp"
#include "platform/opengl/renderer/context.hpp"
#include "platform/opengl/renderer/rendererapi.hpp"
#include "platform/sdl/core/window.hpp"
#include "platform/sdl/input/keyboard.hpp"
#include <SDL.h>
#include <cstdint>
#include <string>

namespace sponge::platform::sdl::core {

struct ApplicationSpecification {
    std::string name = "Sponge";
    uint32_t width = 1600;
    uint32_t height = 900;
    bool fullscreen = true;
    bool vsync = true;
};

using logging::LogItem;

class Application : public sponge::core::Application {
   public:
    explicit Application(ApplicationSpecification specification);

    bool start() override;

    bool iterateLoop() override;

    void shutdown() override;

    bool onUserCreate() override;

    bool onUserUpdate(double elapsedTime) override;

    bool onUserDestroy() override;

    void onEvent(event::Event& event) override;

    void onImGuiRender() const;

    void adjustAspectRatio(uint32_t eventW, uint32_t eventH);

    void pushOverlay(const std::shared_ptr<layer::Layer>& layer) const;

    void pushLayer(const std::shared_ptr<layer::Layer>& layer) const;

    void popLayer(const std::shared_ptr<layer::Layer>& layer) const;

    void popOverlay(const std::shared_ptr<layer::Layer>& layer) const;

    static void setVSync(bool enabled);

    void toggleFullscreen();

    bool isFullscreen() const {
        return fullscreen;
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
        int32_t width;
        int32_t height;
        SDL_GetWindowSize(
            static_cast<SDL_Window*>(sdlWindow->getNativeWindow()), &width,
            &height);
        return static_cast<uint32_t>(height);
    }

    uint32_t getWindowWidth() const {
        int32_t width;
        int32_t height;
        SDL_GetWindowSize(
            static_cast<SDL_Window*>(sdlWindow->getNativeWindow()), &width,
            &height);
        return static_cast<uint32_t>(width);
    }

    static bool hasVerticalSync() {
        return SDL_GL_GetSwapInterval() != 0;
    }

    static void setVerticalSync(const bool value) {
        SDL_GL_SetSwapInterval(value ? 1 : 0);
    }

    std::vector<LogItem>& getMessages() const {
        return *messages;
    }

    void addMessage(const LogItem& item) const {
        messages->emplace_back(item);
    }

    void clearMessages() const {
        messages->clear();
    }

    void setMouseVisible(bool value) const;

    static Application& get() {
        return *instance;
    }

    void run() override;

   private:
    std::string appName = "undefined";
    std::unique_ptr<opengl::renderer::Context> graphics;
    std::unique_ptr<opengl::renderer::RendererAPI> renderer;
    std::unique_ptr<Window> sdlWindow;

    std::unique_ptr<std::vector<LogItem>> messages;

    uint32_t offsetx = 0;
    uint32_t offsety = 0;
    uint32_t w = 0;
    uint32_t h = 0;
    bool fullscreen;

    layer::LayerStack* layerStack;
    input::Keyboard* keyboard;

    void processEvent(const SDL_Event& event, double elapsedTime);

    ApplicationSpecification appSpec;

    static Application* instance;
};

}  // namespace sponge::platform::sdl::core
