#include "platform/sdl/sdlengine.h"

#ifdef EMSCRIPTEN
#include <emscripten/emscripten.h>
#endif

#include <SDL.h>

#include <array>
#include <glm/vec3.hpp>
#include <sstream>

#include "core/keycode.h"
#include "event/applicationevent.h"
#include "event/keyevent.h"
#include "event/mouseevent.h"

namespace Sponge {

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

    SDL_Window *window = SDL_CreateWindow(appName.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
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

    initializeKeyMap();

    return 1;
}

bool SDLEngine::iterateLoop() {
    SDL_Event event;
    uint32_t currentTime;
    uint32_t elapsedTime;

    bool quit = false;
    if (SDL_PollEvent(&event) != 0) {
        processEvent(event);
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
    UNUSED(elapsedTime);
    return true;
}

bool SDLEngine::onUserDestroy() {
    return true;
}

bool SDLEngine::onEvent(Event &event) {
    UNUSED(event);
    return false;
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

void SDLEngine::initializeKeyMap() {
    keymap[SDL_SCANCODE_SPACE] = KeyCode::Space;
    keymap[SDL_SCANCODE_APOSTROPHE] = KeyCode::Apostrophe;
    keymap[SDL_SCANCODE_COMMA] = KeyCode::Comma;
    keymap[SDL_SCANCODE_MINUS] = KeyCode::Minus;
    keymap[SDL_SCANCODE_PERIOD] = KeyCode::Period;
    keymap[SDL_SCANCODE_SLASH] = KeyCode::Slash;

    keymap[SDL_SCANCODE_0] = KeyCode::D0;
    keymap[SDL_SCANCODE_1] = KeyCode::D1;
    keymap[SDL_SCANCODE_2] = KeyCode::D2;
    keymap[SDL_SCANCODE_3] = KeyCode::D3;
    keymap[SDL_SCANCODE_4] = KeyCode::D4;
    keymap[SDL_SCANCODE_5] = KeyCode::D5;
    keymap[SDL_SCANCODE_6] = KeyCode::D6;
    keymap[SDL_SCANCODE_7] = KeyCode::D7;
    keymap[SDL_SCANCODE_8] = KeyCode::D8;
    keymap[SDL_SCANCODE_9] = KeyCode::D9;

    keymap[SDL_SCANCODE_SEMICOLON] = KeyCode::Semicolon;
    keymap[SDL_SCANCODE_EQUALS] = KeyCode::Equal;

    keymap[SDL_SCANCODE_A] = KeyCode::A;
    keymap[SDL_SCANCODE_B] = KeyCode::B;
    keymap[SDL_SCANCODE_C] = KeyCode::C;
    keymap[SDL_SCANCODE_D] = KeyCode::D;
    keymap[SDL_SCANCODE_E] = KeyCode::E;
    keymap[SDL_SCANCODE_F] = KeyCode::F;
    keymap[SDL_SCANCODE_G] = KeyCode::G;
    keymap[SDL_SCANCODE_H] = KeyCode::H;
    keymap[SDL_SCANCODE_I] = KeyCode::I;
    keymap[SDL_SCANCODE_J] = KeyCode::J;
    keymap[SDL_SCANCODE_K] = KeyCode::K;
    keymap[SDL_SCANCODE_L] = KeyCode::L;
    keymap[SDL_SCANCODE_M] = KeyCode::M;
    keymap[SDL_SCANCODE_N] = KeyCode::N;
    keymap[SDL_SCANCODE_O] = KeyCode::O;
    keymap[SDL_SCANCODE_P] = KeyCode::P;
    keymap[SDL_SCANCODE_Q] = KeyCode::Q;
    keymap[SDL_SCANCODE_R] = KeyCode::R;
    keymap[SDL_SCANCODE_S] = KeyCode::S;
    keymap[SDL_SCANCODE_T] = KeyCode::T;
    keymap[SDL_SCANCODE_U] = KeyCode::U;
    keymap[SDL_SCANCODE_V] = KeyCode::V;
    keymap[SDL_SCANCODE_W] = KeyCode::W;
    keymap[SDL_SCANCODE_X] = KeyCode::X;
    keymap[SDL_SCANCODE_Y] = KeyCode::Y;
    keymap[SDL_SCANCODE_Z] = KeyCode::Z;

    keymap[SDL_SCANCODE_LEFTBRACKET] = KeyCode::LeftBracket;
    keymap[SDL_SCANCODE_BACKSLASH] = KeyCode::Backslash;
    keymap[SDL_SCANCODE_RIGHTBRACKET] = KeyCode::RightBracket;
    keymap[SDL_SCANCODE_GRAVE] = KeyCode::GraveAccent;

    keymap[SDL_SCANCODE_INTERNATIONAL1] = KeyCode::World1;
    keymap[SDL_SCANCODE_INTERNATIONAL2] = KeyCode::World2;

    keymap[SDL_SCANCODE_ESCAPE] = KeyCode::Escape;
    keymap[SDL_SCANCODE_RETURN] = KeyCode::Enter;
    keymap[SDL_SCANCODE_TAB] = KeyCode::Tab;
    keymap[SDL_SCANCODE_BACKSPACE] = KeyCode::Backspace;
    keymap[SDL_SCANCODE_INSERT] = KeyCode::Insert;
    keymap[SDL_SCANCODE_DELETE] = KeyCode::Delete;
    keymap[SDL_SCANCODE_RIGHT] = KeyCode::Right;
    keymap[SDL_SCANCODE_LEFT] = KeyCode::Left;
    keymap[SDL_SCANCODE_DOWN] = KeyCode::Down;
    keymap[SDL_SCANCODE_UP] = KeyCode::Up;
    keymap[SDL_SCANCODE_PAGEUP] = KeyCode::PageUp;
    keymap[SDL_SCANCODE_PAGEDOWN] = KeyCode::PageDown;
    keymap[SDL_SCANCODE_HOME] = KeyCode::Home;
    keymap[SDL_SCANCODE_END] = KeyCode::End;
    keymap[SDL_SCANCODE_CAPSLOCK] = KeyCode::CapsLock;
    keymap[SDL_SCANCODE_SCROLLLOCK] = KeyCode::ScrollLock;
    keymap[SDL_SCANCODE_NUMLOCKCLEAR] = KeyCode::NumLock;
    keymap[SDL_SCANCODE_PRINTSCREEN] = KeyCode::PrintScreen;
    keymap[SDL_SCANCODE_PAUSE] = KeyCode::Pause;
    keymap[SDL_SCANCODE_F1] = KeyCode::F1;
    keymap[SDL_SCANCODE_F2] = KeyCode::F2;
    keymap[SDL_SCANCODE_F3] = KeyCode::F3;
    keymap[SDL_SCANCODE_F4] = KeyCode::F4;
    keymap[SDL_SCANCODE_F5] = KeyCode::F5;
    keymap[SDL_SCANCODE_F6] = KeyCode::F6;
    keymap[SDL_SCANCODE_F7] = KeyCode::F7;
    keymap[SDL_SCANCODE_F8] = KeyCode::F8;
    keymap[SDL_SCANCODE_F9] = KeyCode::F9;
    keymap[SDL_SCANCODE_F10] = KeyCode::F10;
    keymap[SDL_SCANCODE_F11] = KeyCode::F11;
    keymap[SDL_SCANCODE_F12] = KeyCode::F12;
    keymap[SDL_SCANCODE_F13] = KeyCode::F13;
    keymap[SDL_SCANCODE_F14] = KeyCode::F14;
    keymap[SDL_SCANCODE_F15] = KeyCode::F15;
    keymap[SDL_SCANCODE_F16] = KeyCode::F16;
    keymap[SDL_SCANCODE_F17] = KeyCode::F17;
    keymap[SDL_SCANCODE_F18] = KeyCode::F18;
    keymap[SDL_SCANCODE_F19] = KeyCode::F19;
    keymap[SDL_SCANCODE_F20] = KeyCode::F20;
    keymap[SDL_SCANCODE_F21] = KeyCode::F21;
    keymap[SDL_SCANCODE_F22] = KeyCode::F22;
    keymap[SDL_SCANCODE_F23] = KeyCode::F23;
    keymap[SDL_SCANCODE_F24] = KeyCode::F24;

    keymap[SDL_SCANCODE_KP_0] = KeyCode::KP0;
    keymap[SDL_SCANCODE_KP_1] = KeyCode::KP1;
    keymap[SDL_SCANCODE_KP_2] = KeyCode::KP2;
    keymap[SDL_SCANCODE_KP_3] = KeyCode::KP3;
    keymap[SDL_SCANCODE_KP_4] = KeyCode::KP4;
    keymap[SDL_SCANCODE_KP_5] = KeyCode::KP5;
    keymap[SDL_SCANCODE_KP_6] = KeyCode::KP6;
    keymap[SDL_SCANCODE_KP_7] = KeyCode::KP7;
    keymap[SDL_SCANCODE_KP_8] = KeyCode::KP8;
    keymap[SDL_SCANCODE_KP_9] = KeyCode::KP9;
    keymap[SDL_SCANCODE_KP_DECIMAL] = KeyCode::KPDecimal;
    keymap[SDL_SCANCODE_KP_DIVIDE] = KeyCode::KPDivide;
    keymap[SDL_SCANCODE_KP_MULTIPLY] = KeyCode::KPMultiply;
    keymap[SDL_SCANCODE_KP_MINUS] = KeyCode::KPSubtract;
    keymap[SDL_SCANCODE_KP_PLUS] = KeyCode::KPAdd;
    keymap[SDL_SCANCODE_KP_ENTER] = KeyCode::KPEnter;
    keymap[SDL_SCANCODE_KP_EQUALS] = KeyCode::KPEqual;

    keymap[SDL_SCANCODE_LSHIFT] = KeyCode::LeftShift;
    keymap[SDL_SCANCODE_LCTRL] = KeyCode::LeftControl;
    keymap[SDL_SCANCODE_LALT] = KeyCode::LeftAlt;
    keymap[SDL_SCANCODE_LGUI] = KeyCode::LeftSuper;
    keymap[SDL_SCANCODE_RSHIFT] = KeyCode::RightShift;
    keymap[SDL_SCANCODE_RCTRL] = KeyCode::RightControl;
    keymap[SDL_SCANCODE_RALT] = KeyCode::RightAlt;
    keymap[SDL_SCANCODE_RGUI] = KeyCode::RightSuper;
    keymap[SDL_SCANCODE_MENU] = KeyCode::Menu;
}

KeyCode SDLEngine::mapScanCodeToKeyCode(const SDL_Scancode &scancode) {
    return keymap[scancode];
}

MouseCode SDLEngine::mapMouseButton(uint8_t index) {
    return index - 1;
}

void SDLEngine::processEvent(SDL_Event &event) {
    if (event.type == SDL_QUIT) {
        auto closeEvent = WindowCloseEvent{};
        onEvent(closeEvent);
    } else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
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
