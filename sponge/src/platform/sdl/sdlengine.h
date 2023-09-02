#pragma once

#include "core/engine.h"
#include "core/keycode.h"
#include "core/layer.h"
#include "core/layerstack.h"
#include "core/mousecode.h"
#include "platform/sdl/sdlwindow.h"
#include <absl/container/flat_hash_map.h>
#include <SDL.h>

namespace sponge {

class SDLEngine : public Engine {
   public:
    SDLEngine();
    bool construct(std::string_view name, uint32_t width, uint32_t height);

    bool start() override;
    bool iterateLoop() override;

    bool onUserCreate() override;
    bool onUserUpdate(uint32_t elapsedTime) override;
    bool onUserDestroy() override;

    void onEvent(Event& event) override;

    void adjustAspectRatio(uint32_t eventW, uint32_t eventH);

    void pushOverlay(Layer* layer);
    void pushLayer(Layer* layer);
    void popLayer(Layer* layer);
    void popOverlay(Layer* layer);

    static void logSDLVersion();

    void toggleFullscreen();

    uint32_t getHeight() {
        return h;
    }
    uint32_t getWidth() {
        return w;
    }

   private:
    std::string appName = "undefined";
    std::unique_ptr<OpenGLContext> graphics;
    std::unique_ptr<OpenGLRendererAPI> renderer;
    std::unique_ptr<SDLWindow> sdlWindow;

    uint32_t offsetx = 0;
    uint32_t offsety = 0;
    uint32_t w = 0;
    uint32_t h = 0;

    uint32_t lastUpdateTime = 0;
    LayerStack* layerStack;
    absl::flat_hash_map<SDL_Scancode, KeyCode> keyCodeMap;

    void initializeKeyCodeMap();
    KeyCode mapScanCodeToKeyCode(const SDL_Scancode& scancode);
    MouseCode mapMouseButton(uint8_t index);
    void processEvent(SDL_Event& event);
};

}  // namespace sponge
