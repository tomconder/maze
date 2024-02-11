#pragma once

#include "core/engine.h"
#include "core/keycode.h"
#include "core/mousecode.h"
#include "event/event.h"
#include "imgui/imguimanager.h"
#include "layer/layer.h"
#include "layer/layerstack.h"
#include "platform/opengl/openglcontext.h"
#include "platform/opengl/openglrendererapi.h"
#include "platform/sdl/sdlwindow.h"
#include <absl/container/flat_hash_map.h>
#include <SDL.h>
#include <cstdint>
#include <memory>
#include <string>

namespace sponge {

class SDLEngine : public Engine {
   public:
    SDLEngine();
    bool construct(std::string_view name, uint32_t width, uint32_t height);

    bool start() override;
    bool iterateLoop() override;
    void shutdown() override;

    bool onUserCreate() override;
    bool onUserUpdate(double elapsedTime) override;
    bool onUserDestroy() override;

    void onEvent(event::Event& event) override;

    void adjustAspectRatio(uint32_t eventW, uint32_t eventH);

    void pushOverlay(const std::shared_ptr<layer::Layer>& layer) const;
    void pushLayer(const std::shared_ptr<layer::Layer>& layer) const;
    void popLayer(const std::shared_ptr<layer::Layer>& layer) const;
    void popOverlay(const std::shared_ptr<layer::Layer>& layer) const;

    static void logSDLVersion();

    void toggleFullscreen() const;

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

    static SDLEngine& get() {
        return *instance;
    }

   private:
#if !NDEBUG
    std::shared_ptr<imgui::ImGuiManager> imguiManager;
#endif
    std::string appName = "undefined";
    std::unique_ptr<graphics::renderer::OpenGLContext> graphics;
    std::unique_ptr<graphics::renderer::OpenGLRendererAPI> renderer;
    std::unique_ptr<SDLWindow> sdlWindow;

    std::unique_ptr<std::vector<LogItem>> messages;

    uint32_t offsetx = 0;
    uint32_t offsety = 0;
    uint32_t w = 0;
    uint32_t h = 0;

    layer::LayerStack* layerStack;
    absl::flat_hash_map<SDL_Scancode, KeyCode> keyCodeMap;

    void initializeKeyCodeMap();
    KeyCode mapScanCodeToKeyCode(const SDL_Scancode& scancode);
    static MouseCode mapMouseButton(uint8_t index);
    void processEvent(const SDL_Event& event, double elapsedTime);

    static SDLEngine* instance;
};

}  // namespace sponge
