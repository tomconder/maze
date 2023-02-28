#include "platform/sdl/sdlengine.h"

#ifdef EMSCRIPTEN
#include <emscripten/emscripten.h>
#endif

#include <SDL.h>

#include <array>
#include <glm/vec3.hpp>
#include <sstream>

#include "event/applicationevent.h"
#include "event/keyevent.h"
#include "event/mouseevent.h"

namespace Sponge {
SDLEngine::SDLEngine() {
    layerStack = new LayerStack();
}

int SDLEngine::construct() const {
    if (w == 0 || h == 0) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, appName.c_str(), "Screen height or width cannot be zero",
                                 nullptr);
        SPONGE_CORE_ERROR("Screen height or width cannot be zero");
        return 0;
    }

    return 1;
}

int SDLEngine::start() {
    SPONGE_CORE_INFO("Initializing SDL");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) < 0) {
        SPONGE_CORE_CRITICAL("Unable to initialize SDL: {}", SDL_GetError());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, appName.c_str(), "Unable to initialize SDL", nullptr);
        return 0;
    }

    logSDLVersion();

    SDL_Window *window =
        SDL_CreateWindow(appName.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h,
                         SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_ALLOW_HIGHDPI);
    if (window == nullptr) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, appName.c_str(), "Could not create window", nullptr);
        SPONGE_CORE_CRITICAL("Could not create window: {}", SDL_GetError());
        return 0;
    }

#ifdef EMSCRIPTEN
    graphics = std::make_unique<OpenGLContext>(window, "OpenGL ES");
#else
    graphics = std::make_unique<OpenGLContext>(window, "OpenGL");
#endif

    graphics->logGlVersion();
    OpenGLContext::logStaticOpenGLInfo();
    OpenGLContext::logGraphicsDriverInfo();
    OpenGLContext::logOpenGLContextInfo();

    renderer = std::make_unique<OpenGLRendererAPI>();
    renderer->init();
    renderer->setClearColor(glm::vec4{ 0.36f, 0.36f, 0.36f, 1.0f });

    if (!onUserCreate()) {
        SDL_DestroyWindow(window);
        return 1;
    }

    lastUpdateTime = SDL_GetTicks();

    SDL_ShowWindow(window);

    return 1;
}

bool SDLEngine::iterateLoop() {
    SDL_Event event;
    uint32_t currentTime;
    uint32_t elapsedTime;

    bool quit = false;
    if (SDL_PollEvent(&event) != 0) {
        processEvent(event);
        if (event.type == SDL_QUIT) {
            quit = true;
        }
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE) {
            quit = true;
        }
    }

    renderer->clear();

    currentTime = SDL_GetTicks();
    elapsedTime = currentTime - lastUpdateTime;
    lastUpdateTime = currentTime;

    if (!onUserUpdate(elapsedTime)) {
        quit = true;
    }

    graphics->flip();

    if (quit && onUserDestroy()) {
#ifdef EMSCRIPTEN
        emscripten_cancel_main_loop();
#endif
        SDL_Quit();

        return true;
    }

    return false;
}

void SDLEngine::logSDLVersion() {
    SDL_version compiled;
    SDL_version linked;

    SDL_VERSION(&compiled)

    std::string revision = SDL_GetRevision();
    std::stringstream ss;
    if (!revision.empty()) {
        ss << "(" << revision << ")";
    }

    SPONGE_CORE_DEBUG("SDL Version (Compiled): {}.{}.{} {}", static_cast<int>(compiled.major),
                      static_cast<int>(compiled.minor), static_cast<int>(compiled.patch),
                      !revision.empty() ? ss.str() : "");

    SDL_GetVersion(&linked);

    SPONGE_CORE_DEBUG("SDL Version (Runtime) : {}.{}.{}", static_cast<int>(linked.major),
                      static_cast<int>(linked.minor), static_cast<int>(linked.patch));
}

bool SDLEngine::onUserCreate() {
    return true;
}

bool SDLEngine::onUserUpdate(uint32_t elapsedTime) {
    bool result = true;

    for (auto layer : *layerStack) {
        if (!layer->onUpdate(elapsedTime)) {
            result = false;
            break;
        }
    }

    return result;
}

bool SDLEngine::onUserDestroy() {
    return true;
}

void SDLEngine::onEvent(Event &event) {
    for (auto it = layerStack->rbegin(); it != layerStack->rend(); ++it) {
        if (event.handled) {
            break;
        }
        (*it)->onEvent(event);
    }
}

void SDLEngine::adjustAspectRatio(int eventW, int eventH) {
    const std::array<glm::vec3, 5> ratios = {
        glm::vec3{ 32.f, 9.f, 32.f / 9.f },    //
        glm::vec3{ 21.f, 9.f, 21.f / 9.f },    //
        glm::vec3{ 16.f, 9.f, 16.f / 9.f },    //
        glm::vec3{ 16.f, 10.f, 16.f / 10.f },  //
        glm::vec3{ 4.f, 3.f, 4.f / 3.f }       //
    };

    // attempt to find the closest matching aspect ratio
    float proposedRatio = static_cast<float>(eventW) / static_cast<float>(eventH);
    auto exceedsRatio = [&proposedRatio](glm::vec3 i) { return proposedRatio >= i.z; };
    auto ratio = std::find_if(begin(ratios), end(ratios), exceedsRatio);
    if (ratio == std::end(ratios)) {
        --ratio;
    }

    // use ratio
    float aspectRatioWidth = ratio->x;
    float aspectRatioHeight = ratio->y;

    float aspectRatio = aspectRatioWidth / aspectRatioHeight;

    auto width = static_cast<float>(eventW);
    auto height = static_cast<float>(eventH);

    float newAspectRatio = width / height;
    if (newAspectRatio > aspectRatio) {
        w = static_cast<int>(aspectRatioWidth * height / aspectRatioHeight);
        h = eventH;
    } else {
        w = eventW;
        h = static_cast<int>(aspectRatioHeight * width / aspectRatioWidth);
    }

    SPONGE_CORE_DEBUG("Resizing to {}x{}", w, h);

    offsetx = (eventW - w) / 2;
    offsety = (eventH - h) / 2;
}

void SDLEngine::pushOverlay(Layer *layer) {
    layerStack->pushOverlay(layer);
    layer->onAttach();
}

void SDLEngine::pushLayer(Layer *layer) {
    layerStack->pushLayer(layer);
    layer->onAttach();
}

KeyCode SDLEngine::mapScanCodeToKeyCode(const SDL_Scancode &scancode) {
    switch (scancode) {
        case SDL_SCANCODE_SPACE:
            return KeyCode::SpongeKey_Space;
        case SDL_SCANCODE_APOSTROPHE:
            return KeyCode::SpongeKey_Apostrophe;
        case SDL_SCANCODE_COMMA:
            return KeyCode::SpongeKey_Comma;
        case SDL_SCANCODE_MINUS:
            return KeyCode::SpongeKey_Minus;
        case SDL_SCANCODE_PERIOD:
            return KeyCode::SpongeKey_Period;
        case SDL_SCANCODE_SLASH:
            return KeyCode::SpongeKey_Slash;

        case SDL_SCANCODE_0:
            return KeyCode::SpongeKey_D0;
        case SDL_SCANCODE_1:
            return KeyCode::SpongeKey_D1;
        case SDL_SCANCODE_2:
            return KeyCode::SpongeKey_D2;
        case SDL_SCANCODE_3:
            return KeyCode::SpongeKey_D3;
        case SDL_SCANCODE_4:
            return KeyCode::SpongeKey_D4;
        case SDL_SCANCODE_5:
            return KeyCode::SpongeKey_D5;
        case SDL_SCANCODE_6:
            return KeyCode::SpongeKey_D6;
        case SDL_SCANCODE_7:
            return KeyCode::SpongeKey_D7;
        case SDL_SCANCODE_8:
            return KeyCode::SpongeKey_D8;
        case SDL_SCANCODE_9:
            return KeyCode::SpongeKey_D9;

        case SDL_SCANCODE_SEMICOLON:
            return KeyCode::SpongeKey_Semicolon;
        case SDL_SCANCODE_EQUALS:
            return KeyCode::SpongeKey_Equal;

        case SDL_SCANCODE_A:
            return KeyCode::SpongeKey_A;
        case SDL_SCANCODE_B:
            return KeyCode::SpongeKey_B;
        case SDL_SCANCODE_C:
            return KeyCode::SpongeKey_C;
        case SDL_SCANCODE_D:
            return KeyCode::SpongeKey_D;
        case SDL_SCANCODE_E:
            return KeyCode::SpongeKey_E;
        case SDL_SCANCODE_F:
            return KeyCode::SpongeKey_F;
        case SDL_SCANCODE_G:
            return KeyCode::SpongeKey_G;
        case SDL_SCANCODE_H:
            return KeyCode::SpongeKey_H;
        case SDL_SCANCODE_I:
            return KeyCode::SpongeKey_I;
        case SDL_SCANCODE_J:
            return KeyCode::SpongeKey_J;
        case SDL_SCANCODE_K:
            return KeyCode::SpongeKey_K;
        case SDL_SCANCODE_L:
            return KeyCode::SpongeKey_L;
        case SDL_SCANCODE_M:
            return KeyCode::SpongeKey_M;
        case SDL_SCANCODE_N:
            return KeyCode::SpongeKey_N;
        case SDL_SCANCODE_O:
            return KeyCode::SpongeKey_O;
        case SDL_SCANCODE_P:
            return KeyCode::SpongeKey_P;
        case SDL_SCANCODE_Q:
            return KeyCode::SpongeKey_Q;
        case SDL_SCANCODE_R:
            return KeyCode::SpongeKey_R;
        case SDL_SCANCODE_S:
            return KeyCode::SpongeKey_S;
        case SDL_SCANCODE_T:
            return KeyCode::SpongeKey_T;
        case SDL_SCANCODE_U:
            return KeyCode::SpongeKey_U;
        case SDL_SCANCODE_V:
            return KeyCode::SpongeKey_V;
        case SDL_SCANCODE_W:
            return KeyCode::SpongeKey_W;
        case SDL_SCANCODE_X:
            return KeyCode::SpongeKey_X;
        case SDL_SCANCODE_Y:
            return KeyCode::SpongeKey_Y;
        case SDL_SCANCODE_Z:
            return KeyCode::SpongeKey_Z;

        case SDL_SCANCODE_LEFTBRACKET:
            return KeyCode::SpongeKey_LeftBracket;
        case SDL_SCANCODE_BACKSLASH:
            return KeyCode::SpongeKey_Backslash;
        case SDL_SCANCODE_RIGHTBRACKET:
            return KeyCode::SpongeKey_RightBracket;
        case SDL_SCANCODE_GRAVE:
            return KeyCode::SpongeKey_GraveAccent;

        case SDL_SCANCODE_INTERNATIONAL1:
            return KeyCode::SpongeKey_World1;
        case SDL_SCANCODE_INTERNATIONAL2:
            return KeyCode::SpongeKey_World2;

        case SDL_SCANCODE_ESCAPE:
            return KeyCode::SpongeKey_Escape;
        case SDL_SCANCODE_RETURN:
            return KeyCode::SpongeKey_Enter;
        case SDL_SCANCODE_TAB:
            return KeyCode::SpongeKey_Tab;
        case SDL_SCANCODE_BACKSPACE:
            return KeyCode::SpongeKey_Backspace;
        case SDL_SCANCODE_INSERT:
            return KeyCode::SpongeKey_Insert;
        case SDL_SCANCODE_DELETE:
            return KeyCode::SpongeKey_Delete;
        case SDL_SCANCODE_RIGHT:
            return KeyCode::SpongeKey_Right;
        case SDL_SCANCODE_LEFT:
            return KeyCode::SpongeKey_Left;
        case SDL_SCANCODE_DOWN:
            return KeyCode::SpongeKey_Down;
        case SDL_SCANCODE_UP:
            return KeyCode::SpongeKey_Up;
        case SDL_SCANCODE_PAGEUP:
            return KeyCode::SpongeKey_PageUp;
        case SDL_SCANCODE_PAGEDOWN:
            return KeyCode::SpongeKey_PageDown;
        case SDL_SCANCODE_HOME:
            return KeyCode::SpongeKey_Home;
        case SDL_SCANCODE_END:
            return KeyCode::SpongeKey_End;
        case SDL_SCANCODE_CAPSLOCK:
            return KeyCode::SpongeKey_CapsLock;
        case SDL_SCANCODE_SCROLLLOCK:
            return KeyCode::SpongeKey_ScrollLock;
        case SDL_SCANCODE_NUMLOCKCLEAR:
            return KeyCode::SpongeKey_NumLock;
        case SDL_SCANCODE_PRINTSCREEN:
            return KeyCode::SpongeKey_PrintScreen;
        case SDL_SCANCODE_PAUSE:
            return KeyCode::SpongeKey_Pause;
        case SDL_SCANCODE_F1:
            return KeyCode::SpongeKey_F1;
        case SDL_SCANCODE_F2:
            return KeyCode::SpongeKey_F2;
        case SDL_SCANCODE_F3:
            return KeyCode::SpongeKey_F3;
        case SDL_SCANCODE_F4:
            return KeyCode::SpongeKey_F4;
        case SDL_SCANCODE_F5:
            return KeyCode::SpongeKey_F5;
        case SDL_SCANCODE_F6:
            return KeyCode::SpongeKey_F6;
        case SDL_SCANCODE_F7:
            return KeyCode::SpongeKey_F7;
        case SDL_SCANCODE_F8:
            return KeyCode::SpongeKey_F8;
        case SDL_SCANCODE_F9:
            return KeyCode::SpongeKey_F9;
        case SDL_SCANCODE_F10:
            return KeyCode::SpongeKey_F10;
        case SDL_SCANCODE_F11:
            return KeyCode::SpongeKey_F11;
        case SDL_SCANCODE_F12:
            return KeyCode::SpongeKey_F12;
        case SDL_SCANCODE_F13:
            return KeyCode::SpongeKey_F13;
        case SDL_SCANCODE_F14:
            return KeyCode::SpongeKey_F14;
        case SDL_SCANCODE_F15:
            return KeyCode::SpongeKey_F15;
        case SDL_SCANCODE_F16:
            return KeyCode::SpongeKey_F16;
        case SDL_SCANCODE_F17:
            return KeyCode::SpongeKey_F17;
        case SDL_SCANCODE_F18:
            return KeyCode::SpongeKey_F18;
        case SDL_SCANCODE_F19:
            return KeyCode::SpongeKey_F19;
        case SDL_SCANCODE_F20:
            return KeyCode::SpongeKey_F20;
        case SDL_SCANCODE_F21:
            return KeyCode::SpongeKey_F21;
        case SDL_SCANCODE_F22:
            return KeyCode::SpongeKey_F22;
        case SDL_SCANCODE_F23:
            return KeyCode::SpongeKey_F23;
        case SDL_SCANCODE_F24:
            return KeyCode::SpongeKey_F24;

        case SDL_SCANCODE_KP_0:
            return KeyCode::SpongeKey_KP0;
        case SDL_SCANCODE_KP_1:
            return KeyCode::SpongeKey_KP1;
        case SDL_SCANCODE_KP_2:
            return KeyCode::SpongeKey_KP2;
        case SDL_SCANCODE_KP_3:
            return KeyCode::SpongeKey_KP3;
        case SDL_SCANCODE_KP_4:
            return KeyCode::SpongeKey_KP4;
        case SDL_SCANCODE_KP_5:
            return KeyCode::SpongeKey_KP5;
        case SDL_SCANCODE_KP_6:
            return KeyCode::SpongeKey_KP6;
        case SDL_SCANCODE_KP_7:
            return KeyCode::SpongeKey_KP7;
        case SDL_SCANCODE_KP_8:
            return KeyCode::SpongeKey_KP8;
        case SDL_SCANCODE_KP_9:
            return KeyCode::SpongeKey_KP9;
        case SDL_SCANCODE_KP_DECIMAL:
            return KeyCode::SpongeKey_KPDecimal;
        case SDL_SCANCODE_KP_DIVIDE:
            return KeyCode::SpongeKey_KPDivide;
        case SDL_SCANCODE_KP_MULTIPLY:
            return KeyCode::SpongeKey_KPMultiply;
        case SDL_SCANCODE_KP_MINUS:
            return KeyCode::SpongeKey_KPSubtract;
        case SDL_SCANCODE_KP_PLUS:
            return KeyCode::SpongeKey_KPAdd;
        case SDL_SCANCODE_KP_ENTER:
            return KeyCode::SpongeKey_KPEnter;
        case SDL_SCANCODE_KP_EQUALS:
            return KeyCode::SpongeKey_KPEqual;

        case SDL_SCANCODE_LSHIFT:
            return KeyCode::SpongeKey_LeftShift;
        case SDL_SCANCODE_LCTRL:
            return KeyCode::SpongeKey_LeftControl;
        case SDL_SCANCODE_LALT:
            return KeyCode::SpongeKey_LeftAlt;
        case SDL_SCANCODE_LGUI:
            return KeyCode::SpongeKey_LeftSuper;
        case SDL_SCANCODE_RSHIFT:
            return KeyCode::SpongeKey_RightShift;
        case SDL_SCANCODE_RCTRL:
            return KeyCode::SpongeKey_RightControl;
        case SDL_SCANCODE_RALT:
            return KeyCode::SpongeKey_RightAlt;
        case SDL_SCANCODE_RGUI:
            return KeyCode::SpongeKey_RightSuper;
        case SDL_SCANCODE_MENU:
            return KeyCode::SpongeKey_Menu;
        default:
            return KeyCode::SpongeKey_None;
    }
}

MouseCode SDLEngine::mapMouseButton(uint8_t index) {
    return index - 1;
}

void SDLEngine::processEvent(SDL_Event &event) {
    if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
        adjustAspectRatio(event.window.data1, event.window.data2);
        renderer->setViewport(offsetx, offsety, w, h);

        auto resizeEvent = WindowResizeEvent{ static_cast<uint32_t>(w), static_cast<uint32_t>(h) };
        onEvent(resizeEvent);
    } else if (event.type == SDL_KEYDOWN) {
        if (event.key.repeat == 0) {
            auto keyEvent = KeyPressedEvent{ mapScanCodeToKeyCode(event.key.keysym.scancode) };
            onEvent(keyEvent);
        }
    } else if (event.type == SDL_KEYUP) {
        auto keyEvent = KeyReleasedEvent{ mapScanCodeToKeyCode(event.key.keysym.scancode) };
        onEvent(keyEvent);
    }

    if (event.type == SDL_MOUSEBUTTONDOWN) {
        auto mouseEvent = MouseButtonPressedEvent{ mapMouseButton(event.button.button) };
        onEvent(mouseEvent);
    } else if (event.type == SDL_MOUSEBUTTONUP) {
        auto mouseEvent = MouseButtonReleasedEvent{ mapMouseButton(event.button.button) };
        onEvent(mouseEvent);
    } else if (event.type == SDL_MOUSEMOTION) {
        auto mouseEvent =
            MouseMovedEvent{ static_cast<float>(event.motion.xrel), static_cast<float>(event.motion.yrel) };
        onEvent(mouseEvent);
    } else if (event.type == SDL_MOUSEWHEEL) {
        auto wheelx = static_cast<float>(event.wheel.preciseX);
        auto wheely = static_cast<float>(event.wheel.preciseY);
#ifdef EMSCRIPTEN
        wheelx /= 100.f;
#endif

        auto mouseEvent = MouseScrolledEvent{ wheelx, wheely };
        onEvent(mouseEvent);
    }
}
}  // namespace Sponge
