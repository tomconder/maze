#pragma once

#include <SDL.h>

#include "core/engine.h"
#include "core/keycode.h"
#include "core/layer.h"
#include "core/layerstack.h"
#include "core/mousecode.h"

namespace Sponge {

class SDLEngine : public Engine {
   public:
    SDLEngine();
    bool construct() const override;

    bool start() override;
    bool iterateLoop() override;

    bool onUserCreate() override;
    bool onUserUpdate(uint32_t elapsedTime) override;
    bool onUserDestroy() override;

    void onEvent(Event &event) override;

    void adjustAspectRatio(int eventW, int eventH);

    void pushOverlay(Layer *layer);
    void pushLayer(Layer *layer);

    static void logSDLVersion();

    std::string appName = "undefined";
    std::unique_ptr<OpenGLContext> graphics;
    std::unique_ptr<OpenGLRendererAPI> renderer;

    int offsetx = 0;
    int offsety = 0;
    int w = 0;
    int h = 0;

   private:
    uint32_t lastUpdateTime = 0;
    Sponge::LayerStack *layerStack;
    std::unordered_map<SDL_Scancode, KeyCode> keyCodeMap;

    void initializeKeyCodeMap();
    KeyCode mapScanCodeToKeyCode(const SDL_Scancode &scancode);
    MouseCode mapMouseButton(uint8_t index);
    void processEvent(SDL_Event &event);
};

}  // namespace Sponge

#define UNUSED(x) (void)(x)
