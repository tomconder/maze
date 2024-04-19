#include "platform/sdl/sdlengine.hpp"
#include "core/log.hpp"
#include "core/timer.hpp"
#include "event/applicationevent.hpp"
#include "event/event.hpp"
#include "event/keyevent.hpp"
#include "event/mouseevent.hpp"
#include "imgui/imguinullmanager.hpp"
#include "input/input.hpp"
#include "layer/layerstack.hpp"
#include "platform/opengl/openglcontext.hpp"
#include "platform/opengl/openglinfo.hpp"
#include "platform/opengl/openglrendererapi.hpp"
#include "platform/sdl/sdlwindow.hpp"
#include <array>
#include <sstream>

namespace sponge {

SDLEngine* SDLEngine::instance = nullptr;
Timer systemTimer;
Timer physicsTimer;

constexpr uint16_t UPDATE_FREQUENCY{ 120 };
constexpr double CYCLE_TIME{ 1.F / UPDATE_FREQUENCY };
double elapsedSeconds{ 0.F };

constexpr auto ratios = std::to_array(
    { glm::vec3{ 32.F, 9.F, 32.F / 9.F }, glm::vec3{ 21.F, 9.F, 21.F / 9.F },
      glm::vec3{ 16.F, 9.F, 16.F / 9.F }, glm::vec3{ 16.F, 10.F, 16.F / 10.F },
      glm::vec3{ 4.F, 3.F, 4.F / 3.F } });

constexpr auto keyCodes =
    std::to_array({ input::keyboard::KeyCode::SpongeKey_W,
                    input::keyboard::KeyCode::SpongeKey_A,
                    input::keyboard::KeyCode::SpongeKey_S,
                    input::keyboard::KeyCode::SpongeKey_D,
                    input::keyboard::KeyCode::SpongeKey_Up,
                    input::keyboard::KeyCode::SpongeKey_Left,
                    input::keyboard::KeyCode::SpongeKey_Down,
                    input::keyboard::KeyCode::SpongeKey_Right });

SDLEngine::SDLEngine() {
    assert(!instance && "Engine already exists!");
    instance = this;

    layerStack = new layer::LayerStack();
    messages = std::make_unique<std::vector<LogItem>>();
    initializeKeyCodeMap();
}

bool SDLEngine::construct(const std::string_view name, const uint32_t width,
                          const uint32_t height) {
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

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
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

    graphics = std::make_unique<renderer::OpenGLContext>(window);

#if !NDEBUG
    imguiManager = std::make_shared<imgui::ImGuiManager>();
#else
    imguiManager = std::make_shared<imgui::ImGuiNullManager>();
#endif
    imguiManager->onAttach();

    renderer::OpenGLInfo::logVersion();
    renderer::OpenGLInfo::logStaticInfo();
    renderer::OpenGLInfo::logGraphicsDriverInfo();
    renderer::OpenGLInfo::logContextInfo();

    sdlWindow->setVSync(true);

    renderer = std::make_unique<renderer::OpenGLRendererAPI>();
    renderer->init();
    renderer->setClearColor(glm::vec4{ 0.36F, 0.36F, 0.36F, 1.0F });

    w = sdlWindow->getWidth();
    h = sdlWindow->getHeight();

    adjustAspectRatio(w, h);
    renderer->setViewport(static_cast<int32_t>(offsetx),
                          static_cast<int32_t>(offsety),
                          static_cast<int32_t>(w), static_cast<int32_t>(h));

    if (!onUserCreate()) {
        return false;
    }

    auto resizeEvent = event::WindowResizeEvent{ w, h };
    onEvent(resizeEvent);

    SDL_ShowWindow(window);

    return true;
}

bool SDLEngine::iterateLoop() {
    SDL_Event event;
    auto quit = false;

    systemTimer.tick();
    elapsedSeconds += systemTimer.getElapsedSeconds();

    if (std::isgreater(elapsedSeconds, CYCLE_TIME)) {
        elapsedSeconds = -CYCLE_TIME;

        physicsTimer.tick();

        while (SDL_PollEvent(&event) != 0) {
            imguiManager->processEvent(&event);

            if (event.type == SDL_QUIT) {
                quit = true;
            }

            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE) {
                quit = true;
            }

            if (imguiManager->isEventHandled() &&
                (event.type == SDL_KEYUP || event.type == SDL_KEYDOWN ||
                 event.type == SDL_TEXTEDITING || event.type == SDL_TEXTINPUT ||
                 event.type == SDL_MOUSEMOTION ||
                 event.type == SDL_MOUSEBUTTONDOWN ||
                 event.type == SDL_MOUSEBUTTONUP ||
                 event.type == SDL_MOUSEWHEEL)) {
                continue;
            }

            processEvent(event, physicsTimer.getElapsedSeconds());
        }

        if (!imguiManager->isEventHandled()) {
            for (const auto& keycode : keyCodes) {
                if (input::Input::isKeyPressed(keycode)) {
                    auto keyPressEvent = event::KeyPressedEvent{
                        keycode, physicsTimer.getElapsedSeconds()
                    };
                    onEvent(keyPressEvent);
                }
            }
        }
    }

    imguiManager->begin();

#if !NDEBUG
    onImGuiRender();
#endif

    renderer->clear();

    if (!onUserUpdate(physicsTimer.getElapsedSeconds())) {
        quit = true;
    }

    if (quit && onUserDestroy()) {
        return true;
    }

    imguiManager->end();

    graphics->flip(sdlWindow->getNativeWindow());

    return false;
}

void SDLEngine::shutdown() {
    imguiManager->onDetach();

    auto* const context = SDL_GL_GetCurrentContext();
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(static_cast<SDL_Window*>(sdlWindow->getNativeWindow()));
    SDL_Quit();
}

void SDLEngine::logSDLVersion() {
    SDL_version compiled;
    SDL_version linked;

    SDL_VERSION(&compiled)

    const std::string revision = SDL_GetRevision();
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

bool SDLEngine::onUserUpdate(const double elapsedTime) {
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

void SDLEngine::onEvent(event::Event& event) {
    for (auto layer = layerStack->rbegin(); layer != layerStack->rend();
         ++layer) {
        if ((*layer)->isActive()) {
            if (event.handled) {
                break;
            }
            (*layer)->onEvent(event);
        }
    }
}

void SDLEngine::onImGuiRender() {
    for (const auto& layer : *layerStack) {
        if (layer->isActive()) {
            layer->onImGuiRender();
        }
    }
}

void SDLEngine::adjustAspectRatio(const uint32_t eventW,
                                  const uint32_t eventH) {
    // attempt to find the closest matching aspect ratio
    float proposedRatio =
        static_cast<float>(eventW) / static_cast<float>(eventH);
    auto exceedsRatio = [&proposedRatio](const glm::vec3 i) {
        return proposedRatio >= i.z;
    };

    auto ratio = std::find_if(begin(ratios), end(ratios), exceedsRatio);
    if (ratio == ratios.end()) {
        ratio = ratios.end() - 1;
    }

    // use ratio
    const float aspectRatioWidth = ratio->x;
    const float aspectRatioHeight = ratio->y;

    const float aspectRatio = aspectRatioWidth / aspectRatioHeight;

    const auto width = static_cast<float>(eventW);
    const auto height = static_cast<float>(eventH);

    if (const float newAspectRatio = width / height;
        newAspectRatio > aspectRatio) {
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

void SDLEngine::pushOverlay(const std::shared_ptr<layer::Layer>& layer) const {
    layerStack->pushOverlay(layer);
    layer->onAttach();
    layer->setActive(true);
}

void SDLEngine::pushLayer(const std::shared_ptr<layer::Layer>& layer) const {
    layerStack->pushLayer(layer);
    layer->onAttach();
    layer->setActive(true);
}

void SDLEngine::popLayer(const std::shared_ptr<layer::Layer>& layer) const {
    layerStack->popLayer(layer);
}

void SDLEngine::popOverlay(const std::shared_ptr<layer::Layer>& layer) const {
    layerStack->popOverlay(layer);
}

void SDLEngine::initializeKeyCodeMap() {
    keyCodeMap[SDL_SCANCODE_SPACE] = input::keyboard::KeyCode::SpongeKey_Space;
    keyCodeMap[SDL_SCANCODE_APOSTROPHE] =
        input::keyboard::KeyCode::SpongeKey_Apostrophe;
    keyCodeMap[SDL_SCANCODE_COMMA] = input::keyboard::KeyCode::SpongeKey_Comma;
    keyCodeMap[SDL_SCANCODE_MINUS] = input::keyboard::KeyCode::SpongeKey_Minus;
    keyCodeMap[SDL_SCANCODE_PERIOD] =
        input::keyboard::KeyCode::SpongeKey_Period;
    keyCodeMap[SDL_SCANCODE_SLASH] = input::keyboard::KeyCode::SpongeKey_Slash;

    keyCodeMap[SDL_SCANCODE_0] = input::keyboard::KeyCode::SpongeKey_D0;
    keyCodeMap[SDL_SCANCODE_1] = input::keyboard::KeyCode::SpongeKey_D1;
    keyCodeMap[SDL_SCANCODE_2] = input::keyboard::KeyCode::SpongeKey_D2;
    keyCodeMap[SDL_SCANCODE_3] = input::keyboard::KeyCode::SpongeKey_D3;
    keyCodeMap[SDL_SCANCODE_4] = input::keyboard::KeyCode::SpongeKey_D4;
    keyCodeMap[SDL_SCANCODE_5] = input::keyboard::KeyCode::SpongeKey_D5;
    keyCodeMap[SDL_SCANCODE_6] = input::keyboard::KeyCode::SpongeKey_D6;
    keyCodeMap[SDL_SCANCODE_7] = input::keyboard::KeyCode::SpongeKey_D7;
    keyCodeMap[SDL_SCANCODE_8] = input::keyboard::KeyCode::SpongeKey_D8;
    keyCodeMap[SDL_SCANCODE_9] = input::keyboard::KeyCode::SpongeKey_D9;

    keyCodeMap[SDL_SCANCODE_SEMICOLON] =
        input::keyboard::KeyCode::SpongeKey_Semicolon;
    keyCodeMap[SDL_SCANCODE_EQUALS] = input::keyboard::KeyCode::SpongeKey_Equal;

    keyCodeMap[SDL_SCANCODE_A] = input::keyboard::KeyCode::SpongeKey_A;
    keyCodeMap[SDL_SCANCODE_B] = input::keyboard::KeyCode::SpongeKey_B;
    keyCodeMap[SDL_SCANCODE_C] = input::keyboard::KeyCode::SpongeKey_C;
    keyCodeMap[SDL_SCANCODE_D] = input::keyboard::KeyCode::SpongeKey_D;
    keyCodeMap[SDL_SCANCODE_E] = input::keyboard::KeyCode::SpongeKey_E;
    keyCodeMap[SDL_SCANCODE_F] = input::keyboard::KeyCode::SpongeKey_F;
    keyCodeMap[SDL_SCANCODE_G] = input::keyboard::KeyCode::SpongeKey_G;
    keyCodeMap[SDL_SCANCODE_H] = input::keyboard::KeyCode::SpongeKey_H;
    keyCodeMap[SDL_SCANCODE_I] = input::keyboard::KeyCode::SpongeKey_I;
    keyCodeMap[SDL_SCANCODE_J] = input::keyboard::KeyCode::SpongeKey_J;
    keyCodeMap[SDL_SCANCODE_K] = input::keyboard::KeyCode::SpongeKey_K;
    keyCodeMap[SDL_SCANCODE_L] = input::keyboard::KeyCode::SpongeKey_L;
    keyCodeMap[SDL_SCANCODE_M] = input::keyboard::KeyCode::SpongeKey_M;
    keyCodeMap[SDL_SCANCODE_N] = input::keyboard::KeyCode::SpongeKey_N;
    keyCodeMap[SDL_SCANCODE_O] = input::keyboard::KeyCode::SpongeKey_O;
    keyCodeMap[SDL_SCANCODE_P] = input::keyboard::KeyCode::SpongeKey_P;
    keyCodeMap[SDL_SCANCODE_Q] = input::keyboard::KeyCode::SpongeKey_Q;
    keyCodeMap[SDL_SCANCODE_R] = input::keyboard::KeyCode::SpongeKey_R;
    keyCodeMap[SDL_SCANCODE_S] = input::keyboard::KeyCode::SpongeKey_S;
    keyCodeMap[SDL_SCANCODE_T] = input::keyboard::KeyCode::SpongeKey_T;
    keyCodeMap[SDL_SCANCODE_U] = input::keyboard::KeyCode::SpongeKey_U;
    keyCodeMap[SDL_SCANCODE_V] = input::keyboard::KeyCode::SpongeKey_V;
    keyCodeMap[SDL_SCANCODE_W] = input::keyboard::KeyCode::SpongeKey_W;
    keyCodeMap[SDL_SCANCODE_X] = input::keyboard::KeyCode::SpongeKey_X;
    keyCodeMap[SDL_SCANCODE_Y] = input::keyboard::KeyCode::SpongeKey_Y;
    keyCodeMap[SDL_SCANCODE_Z] = input::keyboard::KeyCode::SpongeKey_Z;

    keyCodeMap[SDL_SCANCODE_LEFTBRACKET] =
        input::keyboard::KeyCode::SpongeKey_LeftBracket;
    keyCodeMap[SDL_SCANCODE_BACKSLASH] =
        input::keyboard::KeyCode::SpongeKey_Backslash;
    keyCodeMap[SDL_SCANCODE_RIGHTBRACKET] =
        input::keyboard::KeyCode::SpongeKey_RightBracket;
    keyCodeMap[SDL_SCANCODE_GRAVE] =
        input::keyboard::KeyCode::SpongeKey_GraveAccent;

    keyCodeMap[SDL_SCANCODE_INTERNATIONAL1] =
        input::keyboard::KeyCode::SpongeKey_World1;
    keyCodeMap[SDL_SCANCODE_INTERNATIONAL2] =
        input::keyboard::KeyCode::SpongeKey_World2;

    keyCodeMap[SDL_SCANCODE_ESCAPE] =
        input::keyboard::KeyCode::SpongeKey_Escape;
    keyCodeMap[SDL_SCANCODE_RETURN] = input::keyboard::KeyCode::SpongeKey_Enter;
    keyCodeMap[SDL_SCANCODE_TAB] = input::keyboard::KeyCode::SpongeKey_Tab;
    keyCodeMap[SDL_SCANCODE_BACKSPACE] =
        input::keyboard::KeyCode::SpongeKey_Backspace;
    keyCodeMap[SDL_SCANCODE_INSERT] =
        input::keyboard::KeyCode::SpongeKey_Insert;
    keyCodeMap[SDL_SCANCODE_DELETE] =
        input::keyboard::KeyCode::SpongeKey_Delete;
    keyCodeMap[SDL_SCANCODE_RIGHT] = input::keyboard::KeyCode::SpongeKey_Right;
    keyCodeMap[SDL_SCANCODE_LEFT] = input::keyboard::KeyCode::SpongeKey_Left;
    keyCodeMap[SDL_SCANCODE_DOWN] = input::keyboard::KeyCode::SpongeKey_Down;
    keyCodeMap[SDL_SCANCODE_UP] = input::keyboard::KeyCode::SpongeKey_Up;
    keyCodeMap[SDL_SCANCODE_PAGEUP] =
        input::keyboard::KeyCode::SpongeKey_PageUp;
    keyCodeMap[SDL_SCANCODE_PAGEDOWN] =
        input::keyboard::KeyCode::SpongeKey_PageDown;
    keyCodeMap[SDL_SCANCODE_HOME] = input::keyboard::KeyCode::SpongeKey_Home;
    keyCodeMap[SDL_SCANCODE_END] = input::keyboard::KeyCode::SpongeKey_End;
    keyCodeMap[SDL_SCANCODE_CAPSLOCK] =
        input::keyboard::KeyCode::SpongeKey_CapsLock;
    keyCodeMap[SDL_SCANCODE_SCROLLLOCK] =
        input::keyboard::KeyCode::SpongeKey_ScrollLock;
    keyCodeMap[SDL_SCANCODE_NUMLOCKCLEAR] =
        input::keyboard::KeyCode::SpongeKey_NumLock;
    keyCodeMap[SDL_SCANCODE_PRINTSCREEN] =
        input::keyboard::KeyCode::SpongeKey_PrintScreen;
    keyCodeMap[SDL_SCANCODE_PAUSE] = input::keyboard::KeyCode::SpongeKey_Pause;
    keyCodeMap[SDL_SCANCODE_F1] = input::keyboard::KeyCode::SpongeKey_F1;
    keyCodeMap[SDL_SCANCODE_F2] = input::keyboard::KeyCode::SpongeKey_F2;
    keyCodeMap[SDL_SCANCODE_F3] = input::keyboard::KeyCode::SpongeKey_F3;
    keyCodeMap[SDL_SCANCODE_F4] = input::keyboard::KeyCode::SpongeKey_F4;
    keyCodeMap[SDL_SCANCODE_F5] = input::keyboard::KeyCode::SpongeKey_F5;
    keyCodeMap[SDL_SCANCODE_F6] = input::keyboard::KeyCode::SpongeKey_F6;
    keyCodeMap[SDL_SCANCODE_F7] = input::keyboard::KeyCode::SpongeKey_F7;
    keyCodeMap[SDL_SCANCODE_F8] = input::keyboard::KeyCode::SpongeKey_F8;
    keyCodeMap[SDL_SCANCODE_F9] = input::keyboard::KeyCode::SpongeKey_F9;
    keyCodeMap[SDL_SCANCODE_F10] = input::keyboard::KeyCode::SpongeKey_F10;
    keyCodeMap[SDL_SCANCODE_F11] = input::keyboard::KeyCode::SpongeKey_F11;
    keyCodeMap[SDL_SCANCODE_F12] = input::keyboard::KeyCode::SpongeKey_F12;
    keyCodeMap[SDL_SCANCODE_F13] = input::keyboard::KeyCode::SpongeKey_F13;
    keyCodeMap[SDL_SCANCODE_F14] = input::keyboard::KeyCode::SpongeKey_F14;
    keyCodeMap[SDL_SCANCODE_F15] = input::keyboard::KeyCode::SpongeKey_F15;
    keyCodeMap[SDL_SCANCODE_F16] = input::keyboard::KeyCode::SpongeKey_F16;
    keyCodeMap[SDL_SCANCODE_F17] = input::keyboard::KeyCode::SpongeKey_F17;
    keyCodeMap[SDL_SCANCODE_F18] = input::keyboard::KeyCode::SpongeKey_F18;
    keyCodeMap[SDL_SCANCODE_F19] = input::keyboard::KeyCode::SpongeKey_F19;
    keyCodeMap[SDL_SCANCODE_F20] = input::keyboard::KeyCode::SpongeKey_F20;
    keyCodeMap[SDL_SCANCODE_F21] = input::keyboard::KeyCode::SpongeKey_F21;
    keyCodeMap[SDL_SCANCODE_F22] = input::keyboard::KeyCode::SpongeKey_F22;
    keyCodeMap[SDL_SCANCODE_F23] = input::keyboard::KeyCode::SpongeKey_F23;
    keyCodeMap[SDL_SCANCODE_F24] = input::keyboard::KeyCode::SpongeKey_F24;

    keyCodeMap[SDL_SCANCODE_KP_0] = input::keyboard::KeyCode::SpongeKey_KP0;
    keyCodeMap[SDL_SCANCODE_KP_1] = input::keyboard::KeyCode::SpongeKey_KP1;
    keyCodeMap[SDL_SCANCODE_KP_2] = input::keyboard::KeyCode::SpongeKey_KP2;
    keyCodeMap[SDL_SCANCODE_KP_3] = input::keyboard::KeyCode::SpongeKey_KP3;
    keyCodeMap[SDL_SCANCODE_KP_4] = input::keyboard::KeyCode::SpongeKey_KP4;
    keyCodeMap[SDL_SCANCODE_KP_5] = input::keyboard::KeyCode::SpongeKey_KP5;
    keyCodeMap[SDL_SCANCODE_KP_6] = input::keyboard::KeyCode::SpongeKey_KP6;
    keyCodeMap[SDL_SCANCODE_KP_7] = input::keyboard::KeyCode::SpongeKey_KP7;
    keyCodeMap[SDL_SCANCODE_KP_8] = input::keyboard::KeyCode::SpongeKey_KP8;
    keyCodeMap[SDL_SCANCODE_KP_9] = input::keyboard::KeyCode::SpongeKey_KP9;
    keyCodeMap[SDL_SCANCODE_KP_DECIMAL] =
        input::keyboard::KeyCode::SpongeKey_KPDecimal;
    keyCodeMap[SDL_SCANCODE_KP_DIVIDE] =
        input::keyboard::KeyCode::SpongeKey_KPDivide;
    keyCodeMap[SDL_SCANCODE_KP_MULTIPLY] =
        input::keyboard::KeyCode::SpongeKey_KPMultiply;
    keyCodeMap[SDL_SCANCODE_KP_MINUS] =
        input::keyboard::KeyCode::SpongeKey_KPSubtract;
    keyCodeMap[SDL_SCANCODE_KP_PLUS] =
        input::keyboard::KeyCode::SpongeKey_KPAdd;
    keyCodeMap[SDL_SCANCODE_KP_ENTER] =
        input::keyboard::KeyCode::SpongeKey_KPEnter;
    keyCodeMap[SDL_SCANCODE_KP_EQUALS] =
        input::keyboard::KeyCode::SpongeKey_KPEqual;

    keyCodeMap[SDL_SCANCODE_LSHIFT] =
        input::keyboard::KeyCode::SpongeKey_LeftShift;
    keyCodeMap[SDL_SCANCODE_LCTRL] =
        input::keyboard::KeyCode::SpongeKey_LeftControl;
    keyCodeMap[SDL_SCANCODE_LALT] = input::keyboard::KeyCode::SpongeKey_LeftAlt;
    keyCodeMap[SDL_SCANCODE_LGUI] =
        input::keyboard::KeyCode::SpongeKey_LeftSuper;
    keyCodeMap[SDL_SCANCODE_RSHIFT] =
        input::keyboard::KeyCode::SpongeKey_RightShift;
    keyCodeMap[SDL_SCANCODE_RCTRL] =
        input::keyboard::KeyCode::SpongeKey_RightControl;
    keyCodeMap[SDL_SCANCODE_RALT] =
        input::keyboard::KeyCode::SpongeKey_RightAlt;
    keyCodeMap[SDL_SCANCODE_RGUI] =
        input::keyboard::KeyCode::SpongeKey_RightSuper;
    keyCodeMap[SDL_SCANCODE_MENU] = input::keyboard::KeyCode::SpongeKey_Menu;
}

input::keyboard::KeyCode SDLEngine::mapScanCodeToKeyCode(
    const SDL_Scancode& scancode) {
    const auto result = keyCodeMap.find(scancode);
    if (keyCodeMap.find(scancode) == keyCodeMap.end()) {
        return input::keyboard::KeyCode::SpongeKey_None;
    }
    return result->second;
}

input::mouse::MouseCode SDLEngine::mapMouseButton(const uint8_t index) {
    return index - 1;
}

void SDLEngine::setMouseVisible(const bool value) const {
    if (value) {
        SDL_WarpMouseInWindow(
            static_cast<SDL_Window*>(sdlWindow->getNativeWindow()),
            static_cast<int>(getWidth() / 2),
            static_cast<int>(getHeight() / 2));
        SDL_ShowCursor(SDL_TRUE);
        SDL_SetRelativeMouseMode(SDL_FALSE);
    } else {
        SDL_ShowCursor(SDL_FALSE);
        SDL_SetHintWithPriority(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "1",
                                SDL_HINT_OVERRIDE);
        SDL_SetRelativeMouseMode(SDL_TRUE);
    }
}

void SDLEngine::processEvent(const SDL_Event& event, const double elapsedTime) {
    if (event.type == SDL_WINDOWEVENT &&
        event.window.event == SDL_WINDOWEVENT_RESIZED) {
        adjustAspectRatio(event.window.data1, event.window.data2);
        renderer->setViewport(offsetx, offsety, w, h);

        auto resizeEvent = event::WindowResizeEvent{ w, h };
        onEvent(resizeEvent);
    } else if (event.type == SDL_KEYDOWN) {
        auto keyEvent = event::KeyPressedEvent{
            mapScanCodeToKeyCode(event.key.keysym.scancode), elapsedTime
        };
        onEvent(keyEvent);
    } else if (event.type == SDL_KEYUP) {
        auto keyEvent = event::KeyReleasedEvent{ mapScanCodeToKeyCode(
            event.key.keysym.scancode) };
        onEvent(keyEvent);
    }

    if (event.type == SDL_MOUSEBUTTONDOWN) {
        auto mouseEvent = event::MouseButtonPressedEvent{
            mapMouseButton(event.button.button),
            static_cast<float>(event.motion.x),
            static_cast<float>(event.motion.y),
        };
        onEvent(mouseEvent);
    } else if (event.type == SDL_MOUSEBUTTONUP) {
        auto mouseEvent = event::MouseButtonReleasedEvent{ mapMouseButton(
            event.button.button) };
        onEvent(mouseEvent);
    } else if (event.type == SDL_MOUSEMOTION) {
        auto mouseEvent =
            event::MouseMovedEvent{ static_cast<float>(event.motion.xrel),
                                    static_cast<float>(event.motion.yrel),
                                    static_cast<float>(event.motion.x),
                                    static_cast<float>(event.motion.y) };
        onEvent(mouseEvent);
    } else if (event.type == SDL_MOUSEWHEEL) {
        const auto wheelx = event.wheel.preciseX;
        const auto wheely = event.wheel.preciseY;

        auto mouseEvent = event::MouseScrolledEvent{ wheelx, wheely };
        onEvent(mouseEvent);
    }
}

void SDLEngine::toggleFullscreen() const {
    graphics->toggleFullscreen(sdlWindow->getNativeWindow());
}

}  // namespace sponge
