#include "platform/sdl/sdlengine.hpp"
#include "core/log.hpp"
#include "core/timer.hpp"
#include "event/applicationevent.hpp"
#include "event/event.hpp"
#include "event/keyevent.hpp"
#include "event/mouseevent.hpp"
#include "imgui/imguinullmanager.hpp"
#include "layer/layerstack.hpp"
#include "platform/opengl/openglcontext.hpp"
#include "platform/opengl/openglinfo.hpp"
#include "platform/opengl/openglrendererapi.hpp"
#include "platform/sdl/input/sdlkeyboard.hpp"
#include "platform/sdl/input/sdlmouse.hpp"
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

constexpr auto keyCodes = std::to_array(
    { input::KeyCode::SpongeKey_W, input::KeyCode::SpongeKey_A,
      input::KeyCode::SpongeKey_S, input::KeyCode::SpongeKey_D,
      input::KeyCode::SpongeKey_Up, input::KeyCode::SpongeKey_Left,
      input::KeyCode::SpongeKey_Down, input::KeyCode::SpongeKey_Right });

SDLEngine::SDLEngine() {
    assert(!instance && "Engine already exists!");
    instance = this;

    layerStack = new layer::LayerStack();
    messages = std::make_unique<std::vector<LogItem>>();
    keyboard = new input::SDLKeyboard();
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
                if (keyboard->isKeyPressed(keycode)) {
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
        auto keyEvent = event::KeyPressedEvent{ keyboard->mapScanCodeToKeyCode(
                                                    event.key.keysym.scancode),
                                                elapsedTime };
        onEvent(keyEvent);
    } else if (event.type == SDL_KEYUP) {
        auto keyEvent = event::KeyReleasedEvent{ keyboard->mapScanCodeToKeyCode(
            event.key.keysym.scancode) };
        onEvent(keyEvent);
    }

    if (event.type == SDL_MOUSEBUTTONDOWN) {
        auto mouseEvent = event::MouseButtonPressedEvent{
            input::SDLMouse::mapMouseButton(event.button.button),
            static_cast<float>(event.motion.x),
            static_cast<float>(event.motion.y),
        };
        onEvent(mouseEvent);
    } else if (event.type == SDL_MOUSEBUTTONUP) {
        auto mouseEvent = event::MouseButtonReleasedEvent{
            input::SDLMouse::mapMouseButton(event.button.button)
        };
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
