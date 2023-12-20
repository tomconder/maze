#pragma once

#include "core/engine.h"
#include "core/keycode.h"
#include "core/mousecode.h"
#include "event/event.h"
#include "graphics/layer/layer.h"
#include "graphics/layer/layerstack.h"
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
    bool onUserUpdate(uint32_t elapsedTime) override;
    bool onUserDestroy() override;

    void onEvent(event::Event& event) override;

    void adjustAspectRatio(uint32_t eventW, uint32_t eventH);

    void pushOverlay(const std::shared_ptr<graphics::layer::Layer>& layer);
    void pushLayer(const std::shared_ptr<graphics::layer::Layer>& layer);
    void popLayer(const std::shared_ptr<graphics::layer::Layer>& layer);
    void popOverlay(const std::shared_ptr<graphics::layer::Layer>& layer);

    static void logSDLVersion();

    void toggleFullscreen();

    uint32_t getHeight() const {
        return h;
    }
    uint32_t getWidth() const {
        return w;
    }
    void setMouseVisible(bool value);

   private:
    std::string appName = "undefined";
    std::unique_ptr<graphics::renderer::OpenGLContext> graphics;
    std::unique_ptr<graphics::renderer::OpenGLRendererAPI> renderer;
    std::unique_ptr<SDLWindow> sdlWindow;

    uint32_t offsetx = 0;
    uint32_t offsety = 0;
    uint32_t w = 0;
    uint32_t h = 0;

    uint32_t lastUpdateTime = 0;
    graphics::layer::LayerStack* layerStack;
    absl::flat_hash_map<SDL_Scancode, KeyCode> keyCodeMap;

    void initializeKeyCodeMap();
    KeyCode mapScanCodeToKeyCode(const SDL_Scancode& scancode);
    static MouseCode mapMouseButton(uint8_t index);
    void processEvent(const SDL_Event& event, uint32_t elapsedTime);
};

}  // namespace sponge
