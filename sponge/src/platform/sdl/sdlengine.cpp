#include "platform/sdl/sdlengine.h"

#ifdef EMSCRIPTEN
#include <emscripten/emscripten.h>
#endif

#include "event/applicationevent.h"
#include "event/keyevent.h"
#include "event/mouseevent.h"
#include "platform/sdl/sdlwindow.h"
#include <SDL.h>
#include <array>
#include <sstream>

namespace sponge {
SDLEngine::SDLEngine() {
    layerStack = new LayerStack();
    initializeKeyCodeMap();
}

bool SDLEngine::construct(std::string_view name, uint32_t width,
                          uint32_t height) {
    w = width;
    h = height;
    appName = name;

    if (w == 0 || h == 0) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, appName.data(),
                                 "Screen height or width cannot be zero",
                                 nullptr);
        SPONGE_CORE_ERROR("Screen height or width cannot be zero");
        return false;
    }

    return true;
}

bool SDLEngine::start() {
    SPONGE_CORE_INFO("Initializing SDL");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) < 0) {
        SPONGE_CORE_CRITICAL("Unable to initialize SDL: {}", SDL_GetError());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, appName.c_str(),
                                 "Unable to initialize SDL", nullptr);
        return false;
    }

    logSDLVersion();

    WindowProps windowProps;
    windowProps.title = appName;
    windowProps.width = w;
    windowProps.height = h;

    sdlWindow = std::make_unique<SDLWindow>(windowProps);
    auto* window = static_cast<SDL_Window*>(sdlWindow->getNativeWindow());

#ifdef EMSCRIPTEN
    graphics = std::make_unique<OpenGLContext>(window, "OpenGL ES");
#else
    graphics = std::make_unique<OpenGLContext>(window, "OpenGL");
#endif

    graphics->logGlVersion();
    graphics->logStaticOpenGLInfo();
    graphics->logGraphicsDriverInfo();
    graphics->logOpenGLContextInfo();

#ifdef EMSCRIPTEN
    sdlWindow->setVSync(false);
#else
    sdlWindow->setVSync(true);
#endif

    renderer = std::make_unique<OpenGLRendererAPI>();
    renderer->init();
    renderer->setClearColor(glm::vec4{ 0.36F, 0.36F, 0.36F, 1.0F });

    adjustAspectRatio(w, h);
    renderer->setViewport(static_cast<int32_t>(offsetx),
                          static_cast<int32_t>(offsety),
                          static_cast<int32_t>(w), static_cast<int32_t>(h));

    if (!onUserCreate()) {
        return false;
    }

    lastUpdateTime = SDL_GetTicks();

    auto resizeEvent = WindowResizeEvent{ w, h };
    onEvent(resizeEvent);

    SDL_ShowWindow(window);

    SDL_EventState(SDL_TEXTINPUT, SDL_FALSE);

    return true;
}

bool SDLEngine::iterateLoop() {
    SDL_Event event;

    auto quit = false;
    if (SDL_PollEvent(&event) != 0) {
        if (event.type == SDL_QUIT) {
            quit = true;
        }
        if (event.type == SDL_WINDOWEVENT &&
            event.window.event == SDL_WINDOWEVENT_CLOSE) {
            quit = true;
        }
    }

    const auto currentTime = SDL_GetTicks();
    auto elapsedTime = currentTime - lastUpdateTime;
    lastUpdateTime = currentTime;

    processEvent(event, elapsedTime);

    renderer->clear();

    if (!onUserUpdate(elapsedTime)) {
        quit = true;
    }

    if (quit && onUserDestroy()) {
#ifdef EMSCRIPTEN
        emscripten_cancel_main_loop();
#endif
        SDL_Quit();

        return true;
    }

    graphics->flip(sdlWindow->getNativeWindow());

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

    SPONGE_CORE_DEBUG(
        "SDL Version [Compiled]: {}.{}.{} {}", static_cast<int>(compiled.major),
        static_cast<int>(compiled.minor), static_cast<int>(compiled.patch),
        !revision.empty() ? ss.str() : "");

    SDL_GetVersion(&linked);

    SPONGE_CORE_DEBUG(
        "SDL Version [Runtime] : {}.{}.{}", static_cast<int>(linked.major),
        static_cast<int>(linked.minor), static_cast<int>(linked.patch));
}

bool SDLEngine::onUserCreate() {
    return true;
}

bool SDLEngine::onUserUpdate(uint32_t elapsedTime) {
    bool result = true;

    for (const auto& layer : *layerStack) {
        if (layer->isActive()) {
            if (!layer->onUpdate(elapsedTime)) {
                result = false;
                break;
            }
        }
    }

    return result;
}

bool SDLEngine::onUserDestroy() {
    return true;
}

void SDLEngine::onEvent(Event& event) {
    for (auto it = layerStack->rbegin(); it != layerStack->rend(); ++it) {
        if ((*it)->isActive()) {
            if (event.handled) {
                break;
            }
            (*it)->onEvent(event);
        }
    }
}

void SDLEngine::adjustAspectRatio(uint32_t eventW, uint32_t eventH) {
    const std::array<glm::vec3, 5> ratios = {
        glm::vec3{ 32.F, 9.F, 32.F / 9.F },    //
        glm::vec3{ 21.F, 9.F, 21.F / 9.F },    //
        glm::vec3{ 16.F, 9.F, 16.F / 9.F },    //
        glm::vec3{ 16.F, 10.F, 16.F / 10.F },  //
        glm::vec3{ 4.F, 3.F, 4.F / 3.F }       //
    };

    // attempt to find the closest matching aspect ratio
    float proposedRatio =
        static_cast<float>(eventW) / static_cast<float>(eventH);
    auto exceedsRatio = [&proposedRatio](glm::vec3 i) {
        return proposedRatio >= i.z;
    };
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

    if (float newAspectRatio = width / height; newAspectRatio > aspectRatio) {
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

void SDLEngine::pushOverlay(const std::shared_ptr<Layer>& layer) {
    layerStack->pushOverlay(layer);
    layer->onAttach();
    layer->setActive(true);
}

void SDLEngine::pushLayer(const std::shared_ptr<Layer>& layer) {
    layerStack->pushLayer(layer);
    layer->onAttach();
    layer->setActive(true);
}

void SDLEngine::popLayer(const std::shared_ptr<Layer>& layer) {
    layerStack->popLayer(layer);
}

void SDLEngine::popOverlay(const std::shared_ptr<Layer>& layer) {
    layerStack->popOverlay(layer);
}

void SDLEngine::initializeKeyCodeMap() {
    keyCodeMap[SDL_SCANCODE_SPACE] = KeyCode::SpongeKey_Space;
    keyCodeMap[SDL_SCANCODE_APOSTROPHE] = KeyCode::SpongeKey_Apostrophe;
    keyCodeMap[SDL_SCANCODE_COMMA] = KeyCode::SpongeKey_Comma;
    keyCodeMap[SDL_SCANCODE_MINUS] = KeyCode::SpongeKey_Minus;
    keyCodeMap[SDL_SCANCODE_PERIOD] = KeyCode::SpongeKey_Period;
    keyCodeMap[SDL_SCANCODE_SLASH] = KeyCode::SpongeKey_Slash;

    keyCodeMap[SDL_SCANCODE_0] = KeyCode::SpongeKey_D0;
    keyCodeMap[SDL_SCANCODE_1] = KeyCode::SpongeKey_D1;
    keyCodeMap[SDL_SCANCODE_2] = KeyCode::SpongeKey_D2;
    keyCodeMap[SDL_SCANCODE_3] = KeyCode::SpongeKey_D3;
    keyCodeMap[SDL_SCANCODE_4] = KeyCode::SpongeKey_D4;
    keyCodeMap[SDL_SCANCODE_5] = KeyCode::SpongeKey_D5;
    keyCodeMap[SDL_SCANCODE_6] = KeyCode::SpongeKey_D6;
    keyCodeMap[SDL_SCANCODE_7] = KeyCode::SpongeKey_D7;
    keyCodeMap[SDL_SCANCODE_8] = KeyCode::SpongeKey_D8;
    keyCodeMap[SDL_SCANCODE_9] = KeyCode::SpongeKey_D9;

    keyCodeMap[SDL_SCANCODE_SEMICOLON] = KeyCode::SpongeKey_Semicolon;
    keyCodeMap[SDL_SCANCODE_EQUALS] = KeyCode::SpongeKey_Equal;

    keyCodeMap[SDL_SCANCODE_A] = KeyCode::SpongeKey_A;
    keyCodeMap[SDL_SCANCODE_B] = KeyCode::SpongeKey_B;
    keyCodeMap[SDL_SCANCODE_C] = KeyCode::SpongeKey_C;
    keyCodeMap[SDL_SCANCODE_D] = KeyCode::SpongeKey_D;
    keyCodeMap[SDL_SCANCODE_E] = KeyCode::SpongeKey_E;
    keyCodeMap[SDL_SCANCODE_F] = KeyCode::SpongeKey_F;
    keyCodeMap[SDL_SCANCODE_G] = KeyCode::SpongeKey_G;
    keyCodeMap[SDL_SCANCODE_H] = KeyCode::SpongeKey_H;
    keyCodeMap[SDL_SCANCODE_I] = KeyCode::SpongeKey_I;
    keyCodeMap[SDL_SCANCODE_J] = KeyCode::SpongeKey_J;
    keyCodeMap[SDL_SCANCODE_K] = KeyCode::SpongeKey_K;
    keyCodeMap[SDL_SCANCODE_L] = KeyCode::SpongeKey_L;
    keyCodeMap[SDL_SCANCODE_M] = KeyCode::SpongeKey_M;
    keyCodeMap[SDL_SCANCODE_N] = KeyCode::SpongeKey_N;
    keyCodeMap[SDL_SCANCODE_O] = KeyCode::SpongeKey_O;
    keyCodeMap[SDL_SCANCODE_P] = KeyCode::SpongeKey_P;
    keyCodeMap[SDL_SCANCODE_Q] = KeyCode::SpongeKey_Q;
    keyCodeMap[SDL_SCANCODE_R] = KeyCode::SpongeKey_R;
    keyCodeMap[SDL_SCANCODE_S] = KeyCode::SpongeKey_S;
    keyCodeMap[SDL_SCANCODE_T] = KeyCode::SpongeKey_T;
    keyCodeMap[SDL_SCANCODE_U] = KeyCode::SpongeKey_U;
    keyCodeMap[SDL_SCANCODE_V] = KeyCode::SpongeKey_V;
    keyCodeMap[SDL_SCANCODE_W] = KeyCode::SpongeKey_W;
    keyCodeMap[SDL_SCANCODE_X] = KeyCode::SpongeKey_X;
    keyCodeMap[SDL_SCANCODE_Y] = KeyCode::SpongeKey_Y;
    keyCodeMap[SDL_SCANCODE_Z] = KeyCode::SpongeKey_Z;

    keyCodeMap[SDL_SCANCODE_LEFTBRACKET] = KeyCode::SpongeKey_LeftBracket;
    keyCodeMap[SDL_SCANCODE_BACKSLASH] = KeyCode::SpongeKey_Backslash;
    keyCodeMap[SDL_SCANCODE_RIGHTBRACKET] = KeyCode::SpongeKey_RightBracket;
    keyCodeMap[SDL_SCANCODE_GRAVE] = KeyCode::SpongeKey_GraveAccent;

    keyCodeMap[SDL_SCANCODE_INTERNATIONAL1] = KeyCode::SpongeKey_World1;
    keyCodeMap[SDL_SCANCODE_INTERNATIONAL2] = KeyCode::SpongeKey_World2;

    keyCodeMap[SDL_SCANCODE_ESCAPE] = KeyCode::SpongeKey_Escape;
    keyCodeMap[SDL_SCANCODE_RETURN] = KeyCode::SpongeKey_Enter;
    keyCodeMap[SDL_SCANCODE_TAB] = KeyCode::SpongeKey_Tab;
    keyCodeMap[SDL_SCANCODE_BACKSPACE] = KeyCode::SpongeKey_Backspace;
    keyCodeMap[SDL_SCANCODE_INSERT] = KeyCode::SpongeKey_Insert;
    keyCodeMap[SDL_SCANCODE_DELETE] = KeyCode::SpongeKey_Delete;
    keyCodeMap[SDL_SCANCODE_RIGHT] = KeyCode::SpongeKey_Right;
    keyCodeMap[SDL_SCANCODE_LEFT] = KeyCode::SpongeKey_Left;
    keyCodeMap[SDL_SCANCODE_DOWN] = KeyCode::SpongeKey_Down;
    keyCodeMap[SDL_SCANCODE_UP] = KeyCode::SpongeKey_Up;
    keyCodeMap[SDL_SCANCODE_PAGEUP] = KeyCode::SpongeKey_PageUp;
    keyCodeMap[SDL_SCANCODE_PAGEDOWN] = KeyCode::SpongeKey_PageDown;
    keyCodeMap[SDL_SCANCODE_HOME] = KeyCode::SpongeKey_Home;
    keyCodeMap[SDL_SCANCODE_END] = KeyCode::SpongeKey_End;
    keyCodeMap[SDL_SCANCODE_CAPSLOCK] = KeyCode::SpongeKey_CapsLock;
    keyCodeMap[SDL_SCANCODE_SCROLLLOCK] = KeyCode::SpongeKey_ScrollLock;
    keyCodeMap[SDL_SCANCODE_NUMLOCKCLEAR] = KeyCode::SpongeKey_NumLock;
    keyCodeMap[SDL_SCANCODE_PRINTSCREEN] = KeyCode::SpongeKey_PrintScreen;
    keyCodeMap[SDL_SCANCODE_PAUSE] = KeyCode::SpongeKey_Pause;
    keyCodeMap[SDL_SCANCODE_F1] = KeyCode::SpongeKey_F1;
    keyCodeMap[SDL_SCANCODE_F2] = KeyCode::SpongeKey_F2;
    keyCodeMap[SDL_SCANCODE_F3] = KeyCode::SpongeKey_F3;
    keyCodeMap[SDL_SCANCODE_F4] = KeyCode::SpongeKey_F4;
    keyCodeMap[SDL_SCANCODE_F5] = KeyCode::SpongeKey_F5;
    keyCodeMap[SDL_SCANCODE_F6] = KeyCode::SpongeKey_F6;
    keyCodeMap[SDL_SCANCODE_F7] = KeyCode::SpongeKey_F7;
    keyCodeMap[SDL_SCANCODE_F8] = KeyCode::SpongeKey_F8;
    keyCodeMap[SDL_SCANCODE_F9] = KeyCode::SpongeKey_F9;
    keyCodeMap[SDL_SCANCODE_F10] = KeyCode::SpongeKey_F10;
    keyCodeMap[SDL_SCANCODE_F11] = KeyCode::SpongeKey_F11;
    keyCodeMap[SDL_SCANCODE_F12] = KeyCode::SpongeKey_F12;
    keyCodeMap[SDL_SCANCODE_F13] = KeyCode::SpongeKey_F13;
    keyCodeMap[SDL_SCANCODE_F14] = KeyCode::SpongeKey_F14;
    keyCodeMap[SDL_SCANCODE_F15] = KeyCode::SpongeKey_F15;
    keyCodeMap[SDL_SCANCODE_F16] = KeyCode::SpongeKey_F16;
    keyCodeMap[SDL_SCANCODE_F17] = KeyCode::SpongeKey_F17;
    keyCodeMap[SDL_SCANCODE_F18] = KeyCode::SpongeKey_F18;
    keyCodeMap[SDL_SCANCODE_F19] = KeyCode::SpongeKey_F19;
    keyCodeMap[SDL_SCANCODE_F20] = KeyCode::SpongeKey_F20;
    keyCodeMap[SDL_SCANCODE_F21] = KeyCode::SpongeKey_F21;
    keyCodeMap[SDL_SCANCODE_F22] = KeyCode::SpongeKey_F22;
    keyCodeMap[SDL_SCANCODE_F23] = KeyCode::SpongeKey_F23;
    keyCodeMap[SDL_SCANCODE_F24] = KeyCode::SpongeKey_F24;

    keyCodeMap[SDL_SCANCODE_KP_0] = KeyCode::SpongeKey_KP0;
    keyCodeMap[SDL_SCANCODE_KP_1] = KeyCode::SpongeKey_KP1;
    keyCodeMap[SDL_SCANCODE_KP_2] = KeyCode::SpongeKey_KP2;
    keyCodeMap[SDL_SCANCODE_KP_3] = KeyCode::SpongeKey_KP3;
    keyCodeMap[SDL_SCANCODE_KP_4] = KeyCode::SpongeKey_KP4;
    keyCodeMap[SDL_SCANCODE_KP_5] = KeyCode::SpongeKey_KP5;
    keyCodeMap[SDL_SCANCODE_KP_6] = KeyCode::SpongeKey_KP6;
    keyCodeMap[SDL_SCANCODE_KP_7] = KeyCode::SpongeKey_KP7;
    keyCodeMap[SDL_SCANCODE_KP_8] = KeyCode::SpongeKey_KP8;
    keyCodeMap[SDL_SCANCODE_KP_9] = KeyCode::SpongeKey_KP9;
    keyCodeMap[SDL_SCANCODE_KP_DECIMAL] = KeyCode::SpongeKey_KPDecimal;
    keyCodeMap[SDL_SCANCODE_KP_DIVIDE] = KeyCode::SpongeKey_KPDivide;
    keyCodeMap[SDL_SCANCODE_KP_MULTIPLY] = KeyCode::SpongeKey_KPMultiply;
    keyCodeMap[SDL_SCANCODE_KP_MINUS] = KeyCode::SpongeKey_KPSubtract;
    keyCodeMap[SDL_SCANCODE_KP_PLUS] = KeyCode::SpongeKey_KPAdd;
    keyCodeMap[SDL_SCANCODE_KP_ENTER] = KeyCode::SpongeKey_KPEnter;
    keyCodeMap[SDL_SCANCODE_KP_EQUALS] = KeyCode::SpongeKey_KPEqual;

    keyCodeMap[SDL_SCANCODE_LSHIFT] = KeyCode::SpongeKey_LeftShift;
    keyCodeMap[SDL_SCANCODE_LCTRL] = KeyCode::SpongeKey_LeftControl;
    keyCodeMap[SDL_SCANCODE_LALT] = KeyCode::SpongeKey_LeftAlt;
    keyCodeMap[SDL_SCANCODE_LGUI] = KeyCode::SpongeKey_LeftSuper;
    keyCodeMap[SDL_SCANCODE_RSHIFT] = KeyCode::SpongeKey_RightShift;
    keyCodeMap[SDL_SCANCODE_RCTRL] = KeyCode::SpongeKey_RightControl;
    keyCodeMap[SDL_SCANCODE_RALT] = KeyCode::SpongeKey_RightAlt;
    keyCodeMap[SDL_SCANCODE_RGUI] = KeyCode::SpongeKey_RightSuper;
    keyCodeMap[SDL_SCANCODE_MENU] = KeyCode::SpongeKey_Menu;
}

KeyCode SDLEngine::mapScanCodeToKeyCode(const SDL_Scancode& scancode) {
    auto result = keyCodeMap.find(scancode);
    if (keyCodeMap.find(scancode) == keyCodeMap.end()) {
        return KeyCode::SpongeKey_None;
    }
    return result->second;
}

MouseCode SDLEngine::mapMouseButton(const uint8_t index) {
    return index - 1;
}

void SDLEngine::processEvent(const SDL_Event& event,
                             const uint32_t elapsedTime) {
    if (event.type == SDL_WINDOWEVENT &&
        event.window.event == SDL_WINDOWEVENT_RESIZED) {
        adjustAspectRatio(event.window.data1, event.window.data2);
        renderer->setViewport(offsetx, offsety, w, h);

        auto resizeEvent = WindowResizeEvent{ w, h };
        onEvent(resizeEvent);
    } else if (event.type == SDL_KEYDOWN) {
        if (event.key.repeat == 0) {
            auto keyEvent = KeyPressedEvent{
                mapScanCodeToKeyCode(event.key.keysym.scancode), elapsedTime
            };
            onEvent(keyEvent);
        }
    } else if (event.type == SDL_KEYUP) {
        auto keyEvent =
            KeyReleasedEvent{ mapScanCodeToKeyCode(event.key.keysym.scancode) };
        onEvent(keyEvent);
    }

    if (event.type == SDL_MOUSEBUTTONDOWN) {
        auto mouseEvent = MouseButtonPressedEvent{
            mapMouseButton(event.button.button),
            static_cast<float>(event.motion.x),
            static_cast<float>(event.motion.y),
        };
        onEvent(mouseEvent);
    } else if (event.type == SDL_MOUSEBUTTONUP) {
        auto mouseEvent =
            MouseButtonReleasedEvent{ mapMouseButton(event.button.button) };
        onEvent(mouseEvent);
    } else if (event.type == SDL_MOUSEMOTION) {
        auto mouseEvent =
            MouseMovedEvent{ static_cast<float>(event.motion.xrel),
                             static_cast<float>(event.motion.yrel),
                             static_cast<float>(event.motion.x),
                             static_cast<float>(event.motion.y) };
        onEvent(mouseEvent);
    } else if (event.type == SDL_MOUSEWHEEL) {
        auto wheelx = event.wheel.preciseX;
        auto wheely = event.wheel.preciseY;
#ifdef EMSCRIPTEN
        wheelx /= 100.f;
#endif

        auto mouseEvent = MouseScrolledEvent{ wheelx, wheely };
        onEvent(mouseEvent);
    }
}

void SDLEngine::toggleFullscreen() {
    graphics->toggleFullscreen(sdlWindow->getNativeWindow());
}

}  // namespace sponge
