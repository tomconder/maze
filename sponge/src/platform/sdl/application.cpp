#include "application.hpp"
#include "core/timer.hpp"
#include "event/applicationevent.hpp"
#include "event/event.hpp"
#include "event/keyevent.hpp"
#include "event/mouseevent.hpp"
#include "info.hpp"
#include "layer/layerstack.hpp"
#include "logging/log.hpp"
#include "platform/opengl/context.hpp"
#include "platform/opengl/info.hpp"
#include "platform/opengl/rendererapi.hpp"
#include "platform/sdl/imgui/noopmanager.hpp"
#include "platform/sdl/imgui/sdlmanager.hpp"
#include "platform/sdl/input/keyboard.hpp"
#include "platform/sdl/input/mouse.hpp"
#include "platform/sdl/logging/sink.hpp"
#include "platform/sdl/window.hpp"
#include <array>

namespace sponge::platform::sdl {

auto sdlManager = std::make_shared<imgui::SDLManager>();
auto noopManager = std::make_shared<imgui::NoopManager>();

#if !NDEBUG
auto imguiManager = sdlManager;
#else
auto imguiManager = noopManager;
#endif

Application* Application::instance = nullptr;
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
    { sponge::input::KeyCode::SpongeKey_W, sponge::input::KeyCode::SpongeKey_A,
      sponge::input::KeyCode::SpongeKey_S, sponge::input::KeyCode::SpongeKey_D,
      sponge::input::KeyCode::SpongeKey_Up,
      sponge::input::KeyCode::SpongeKey_Left,
      sponge::input::KeyCode::SpongeKey_Down,
      sponge::input::KeyCode::SpongeKey_Right });

Application::Application() {
    const auto guiSink =
        std::make_shared<platform::sdl::imgui::Sink<std::mutex>>();
    logging::Log::addSink(guiSink, logging::Log::guiFormatPattern);

    assert(!instance && "Application already exists!");
    instance = this;

    layerStack = new layer::LayerStack();
    messages = std::make_unique<std::vector<logging::LogItem>>();
    keyboard = new platform::sdl::input::Keyboard();
}

bool Application::construct(const std::string_view name, const uint32_t width,
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

bool Application::start() {
    SPONGE_CORE_INFO("Initializing SDL");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        SPONGE_CORE_CRITICAL("Unable to initialize SDL: {}", SDL_GetError());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, appName.c_str(),
                                 "Unable to initialize SDL", nullptr);
        return false;
    }

    Info::logVersion();

    WindowProps windowProps;
    windowProps.title = appName;
    windowProps.width = w;
    windowProps.height = h;

    sdlWindow = std::make_unique<Window>(windowProps);
    auto* window = static_cast<SDL_Window*>(sdlWindow->getNativeWindow());

    graphics = std::make_unique<opengl::Context>(window);

    imguiManager->onAttach();

    opengl::Info::logVersion();
    opengl::Info::logStaticInfo();
    opengl::Info::logGraphicsDriverInfo();
    opengl::Info::logContextInfo();

    sdlWindow->setVSync(true);

    renderer = std::make_unique<opengl::RendererAPI>();
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

bool Application::iterateLoop() {
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

void Application::shutdown() {
    imguiManager->onDetach();

    auto* const context = SDL_GL_GetCurrentContext();
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(static_cast<SDL_Window*>(sdlWindow->getNativeWindow()));
    SDL_Quit();
}

bool Application::onUserCreate() {
    return true;
}

bool Application::onUserUpdate(const double elapsedTime) {
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

bool Application::onUserDestroy() {
    return true;
}

void Application::onEvent(event::Event& event) {
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

void Application::onImGuiRender() const {
    for (const auto& layer : *layerStack) {
        if (layer->isActive()) {
            layer->onImGuiRender();
        }
    }
}

void Application::adjustAspectRatio(const uint32_t eventW,
                                    const uint32_t eventH) {
    // attempt to find the closest matching aspect ratio
    float proposedRatio =
        static_cast<float>(eventW) / static_cast<float>(eventH);
    auto exceedsRatio = [&proposedRatio](const glm::vec3 i) {
        return proposedRatio >= i.z;
    };

    glm::vec3 ratio;
    if (const auto it = std::find_if(begin(ratios), end(ratios), exceedsRatio);
        it != std::end(ratios)) {
        ratio = *it;
    } else {
        ratio = *(ratios.end() - 1);
    }

    // use ratio
    const float aspectRatioWidth = ratio.x;
    const float aspectRatioHeight = ratio.y;

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

void Application::pushOverlay(
    const std::shared_ptr<layer::Layer>& layer) const {
    layerStack->pushOverlay(layer);
    layer->onAttach();
    layer->setActive(true);
}

void Application::pushLayer(const std::shared_ptr<layer::Layer>& layer) const {
    layerStack->pushLayer(layer);
    layer->onAttach();
    layer->setActive(true);
}

void Application::popLayer(const std::shared_ptr<layer::Layer>& layer) const {
    layerStack->popLayer(layer);
}

void Application::popOverlay(const std::shared_ptr<layer::Layer>& layer) const {
    layerStack->popOverlay(layer);
}

void Application::setMouseVisible(const bool value) const {
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

void Application::processEvent(const SDL_Event& event,
                               const double elapsedTime) {
    if (event.type == SDL_WINDOWEVENT &&
        event.window.event == SDL_WINDOWEVENT_RESIZED) {
        adjustAspectRatio(event.window.data1, event.window.data2);
        renderer->setViewport(offsetx, offsety, w, h);

        auto resizeEvent = event::WindowResizeEvent{ w, h };
        onEvent(resizeEvent);
    } else if (event.type == SDL_KEYDOWN) {
        auto keyEvent = event::KeyPressedEvent{
            input::Keyboard::mapScanCodeToKeyCode(event.key.keysym.scancode),
            elapsedTime
        };
        onEvent(keyEvent);
    } else if (event.type == SDL_KEYUP) {
        auto keyEvent = event::KeyReleasedEvent{
            input::Keyboard::mapScanCodeToKeyCode(event.key.keysym.scancode)
        };
        onEvent(keyEvent);
    }

    if (event.type == SDL_MOUSEBUTTONDOWN) {
        auto mouseEvent = event::MouseButtonPressedEvent{
            platform::sdl::input::Mouse::mapMouseButton(event.button.button),
            static_cast<float>(event.motion.x),
            static_cast<float>(event.motion.y),
        };
        onEvent(mouseEvent);
    } else if (event.type == SDL_MOUSEBUTTONUP) {
        auto mouseEvent = event::MouseButtonReleasedEvent{
            platform::sdl::input::Mouse::mapMouseButton(event.button.button)
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

void Application::toggleFullscreen() const {
    graphics->toggleFullscreen(sdlWindow->getNativeWindow());
}

}  // namespace sponge::platform::sdl
