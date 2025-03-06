#include "application.hpp"
#include "core/timer.hpp"
#include "debug/profiler.hpp"
#include "event/applicationevent.hpp"
#include "input/keycode.hpp"
#include "logging/log.hpp"
#include "platform/glfw/core/input.hpp"
#include "platform/glfw/logging/sink.hpp"
#include "platform/opengl/renderer/context.hpp"
#include "platform/opengl/renderer/rendererapi.hpp"
#include <glm/glm.hpp>
#include <array>
#include <ranges>
#include <utility>

using sponge::input::KeyCode;

namespace {
sponge::core::Timer systemTimer;

constexpr uint32_t defaultWidth{ 1600 };
constexpr uint32_t defaultHeight{ 900 };

constexpr auto ratios = std::to_array(
    { glm::vec3{ 32.F, 9.F, 32.F / 9.F }, glm::vec3{ 21.F, 9.F, 21.F / 9.F },
      glm::vec3{ 16.F, 9.F, 16.F / 9.F }, glm::vec3{ 16.F, 10.F, 16.F / 10.F },
      glm::vec3{ 4.F, 3.F, 4.F / 3.F } });
}  // namespace

namespace sponge::platform::glfw::core {
Application* Application::instance = nullptr;

Application::Application(ApplicationSpecification specification)
    : appSpec(std::move(specification)) {
    const auto guiSink = std::make_shared<imgui::Sink<std::mutex>>();
    logging::Log::addSink(guiSink, logging::Log::guiFormatPattern);

    assert(!instance && "Application already exists!");
    instance = this;

    layerStack = new layer::LayerStack();
    messages = std::make_unique<std::vector<LogItem>>();
}

Application::~Application() {
    delete layerStack;
}

bool Application::start() {
    SPONGE_PROFILE_SECTION("Application::start");

    appName = appSpec.name;

    if (!appSpec.fullscreen && (appSpec.width == 0 || appSpec.height == 0)) {
        SPONGE_CORE_ERROR("Screen height or width cannot be zero");
        return false;
    }

    SPONGE_CORE_INFO("Initializing glfw");

    if (glfwInit() == GLFW_FALSE) {
        const char* description;
        glfwGetError(&description);
        SPONGE_CORE_CRITICAL("Unable to initialize glfw: {}", description);
        return false;
    }

    fullscreen = appSpec.fullscreen;

    graphics = std::make_unique<opengl::renderer::Context>();

    window = std::make_unique<Window>(
        sponge::core::WindowProps{ .title = appName,
                                   .width = appSpec.width,
                                   .height = appSpec.height,
                                   .fullscreen = appSpec.fullscreen });
    auto* glfwWindow = static_cast<GLFWwindow*>(window->getNativeWindow());

    graphics->init(glfwWindow);

    imguiManager->onAttach(static_cast<GLFWwindow*>(window->getNativeWindow()));

    setVerticalSync(appSpec.vsync);

    renderer = std::make_unique<opengl::renderer::RendererAPI>();
    renderer->init();
    renderer->setClearColor(glm::vec4{ 0.36F, 0.36F, 0.36F, 1.0F });

    w = window->getWidth();
    h = window->getHeight();

    window->setEventCallback([this](event::Event& e) { onEvent(e); });

    adjustAspectRatio(w, h);

    if (!onUserCreate()) {
        return false;
    }

    auto resizeEvent = event::WindowResizeEvent{ w, h };
    onEvent(resizeEvent);

    glfwShowWindow(static_cast<GLFWwindow*>(window->getNativeWindow()));

    return true;
}

bool Application::iterateLoop() {
    SPONGE_PROFILE_SECTION("Application:iterateLoop");

    auto quit = false;

    systemTimer.tick();

    glfwPollEvents();

    imguiManager->begin();

#ifdef ENABLE_IMGUI
    onImGuiRender();
#endif

    renderer->clear();

    if (!onUserUpdate(systemTimer.getElapsedSeconds())) {
        quit = true;
    }

    if (quit && onUserDestroy()) {
        return true;
    }

    imguiManager->end();

    graphics->flip(window->getNativeWindow());

    return false;
}

void Application::shutdown() {
    imguiManager->onDetach();
}

bool Application::onUserCreate() {
    return true;
}

bool Application::onUserUpdate(const double elapsedTime) {
    SPONGE_PROFILE_SECTION("Application::onUserUpdate");

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
    SPONGE_PROFILE_SECTION("Application::onEvent");

    for (const auto& layer : std::ranges::reverse_view(*layerStack)) {
        if (layer->isActive() && !event.handled) {
            layer->onEvent(event);
        }
    }
}

void Application::onImGuiRender() const {
    SPONGE_PROFILE_SECTION("Application::onImGuiRender");

    for (const auto& layer : *layerStack) {
        if (layer->isActive()) {
            layer->onImGuiRender();
        }
    }
}

void Application::adjustAspectRatio(const uint32_t eventW,
                                    const uint32_t eventH) {
    float proposedRatio =
        static_cast<float>(eventW) / static_cast<float>(eventH);
    auto exceedsRatio = [&proposedRatio](const glm::vec3 i) {
        return proposedRatio >= i.z;
    };

    glm::vec3 ratio;
    if (const auto it = std::ranges::find_if(ratios, exceedsRatio);
        it != std::end(ratios)) {
        ratio = *it;
    } else {
        ratio = *(ratios.end() - 1);
    }

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

    SPONGE_CORE_DEBUG("Resizing viewport to {}x{}", w, h);

    offsetx = (eventW - w) / 2;
    offsety = (eventH - h) / 2;

    renderer->setViewport(static_cast<int32_t>(offsetx),
                          static_cast<int32_t>(offsety),
                          static_cast<int32_t>(w), static_cast<int32_t>(h));
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

void Application::toggleFullscreen() {
    auto* glfwWindow = static_cast<GLFWwindow*>(window->getNativeWindow());
    if (!glfwWindow) {
        SPONGE_CORE_WARN("Window handle is null");
        return;
    }

    fullscreen = !fullscreen;

    if (fullscreen) {
        glfwGetWindowPos(glfwWindow, &prevX, &prevY);
        glfwGetWindowSize(glfwWindow, &prevW, &prevH);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(glfwWindow, monitor, 0, 0, mode->width,
                             mode->height, mode->refreshRate);
    } else {
        if (prevW <= 0) {
            prevW = defaultWidth;
        }
        if (prevH <= 0) {
            prevH = defaultHeight;
        }

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        if (prevX <= 0) {
            prevX = prevW - mode->width / 2;
        }
        if (prevY <= 0) {
            prevY = prevH - mode->height / 2;
        }

        glfwSetWindowAttrib(glfwWindow, GLFW_DECORATED, GLFW_TRUE);
        glfwSetWindowMonitor(glfwWindow, nullptr, prevX, prevY, prevW, prevH,
                             GLFW_DONT_CARE);
        glfwFocusWindow(glfwWindow);
    }
}

void Application::setMouseVisible(const bool value) const {
    auto* glfwWindow = static_cast<GLFWwindow*>(window->getNativeWindow());
    if (value) {
        glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    } else {
        glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
}

void Application::centerMouse() const {
    auto* glfwWindow = static_cast<GLFWwindow*>(window->getNativeWindow());
    glfwSetCursorPos(glfwWindow, w / 2.F, h / 2.F);
    Input::setPrevCursorPos({ w / 2.F, h / 2.F });
}

void Application::run() {
#ifdef ENABLE_PROFILING
    SPONGE_DEBUG("Tracy profiler enabled");
#endif

    SPONGE_INFO("Starting");

    if (!start()) {
        return;
    }

    SPONGE_INFO("Iterating loop");

    bool quit = false;
    while (!quit) {
        quit = iterateLoop();
    }

    SPONGE_INFO("Shutting down");
    shutdown();
}
}  // namespace sponge::platform::glfw::core
