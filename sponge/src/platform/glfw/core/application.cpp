#include "platform/glfw/core/application.hpp"

#include "core/timer.hpp"
#include "debug/profiler.hpp"
#include "event/applicationevent.hpp"
#include "logging/log.hpp"
#include "platform/glfw/core/input.hpp"
#include "platform/glfw/logging/sink.hpp"
#include "platform/opengl/debug/diagnostics.hpp"
#include "platform/opengl/renderer/context.hpp"
#include "platform/opengl/renderer/rendererapi.hpp"

#include <fmt/format.h>
#include <glm/glm.hpp>

#include <array>
#include <memory>
#include <ranges>
#include <utility>

namespace {
sponge::core::Timer systemTimer;

constexpr uint32_t defaultWidth{ 1600 };
constexpr uint32_t defaultHeight{ 900 };
}  // namespace

namespace sponge::platform::glfw::core {
using opengl::debug::Diagnostics;

Application* Application::instance = nullptr;

Application::Application(ApplicationSpecification specification) :
    appSpec(std::move(specification)) {
    const auto guiSink = std::make_shared<imgui::Sink<std::mutex>>();
    logging::Log::addSink(guiSink, logging::Log::guiFormatPattern);

    assert(!instance && "Application already exists!");
    instance = this;

    layerStack = std::make_unique<layer::LayerStack>();
    messages   = std::make_unique<std::vector<LogItem>>();
}

Application::~Application() {
    instance = nullptr;
}

bool Application::start() {
    SPONGE_PROFILE_SECTION("Application::start");

    appName = appSpec.name;

    SPONGE_CORE_INFO("Initializing glfw");

    if (glfwInit() == GLFW_FALSE) {
        const char* descCStr = nullptr;
        glfwGetError(&descCStr);
        const std::string description = descCStr ? descCStr : "Unknown error";
        SPONGE_CORE_CRITICAL(
            fmt::format("Unable to initialize glfw: {}", description));
        return false;
    }

    fullscreen = appSpec.fullscreen;

    graphics = std::make_unique<opengl::renderer::Context>();

    window = std::make_unique<Window>(
        sponge::core::WindowProps{ .title      = appName,
                                   .width      = appSpec.width,
                                   .height     = appSpec.height,
                                   .fullscreen = appSpec.fullscreen });
    auto* glfwWindow = static_cast<GLFWwindow*>(window->getNativeWindow());

    graphics->init(glfwWindow);

    Diagnostics::log();

    imguiManager->onAttach(static_cast<GLFWwindow*>(window->getNativeWindow()));

    setVerticalSync(appSpec.vsync);

    renderer = std::make_unique<opengl::renderer::RendererAPI>();
    renderer->init();
    renderer->setClearColor(glm::vec4{ 0.36F, 0.36F, 0.36F, 1.0F });

    window->setEventCallback([this](event::Event& e) { onEvent(e); });

    const auto [w, h] =
        adjustAspectRatio(window->getWidth(), window->getHeight());

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
        layer->onEvent(event);
        if (event.handled) {
            break;
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

std::tuple<uint32_t, uint32_t>
    Application::adjustAspectRatio(const uint32_t eventW,
                                   const uint32_t eventH) const {
    const auto [width, height, offsetX, offsetY] =
        window->adjustAspectRatio(eventW, eventH);

    renderer->setViewport(
        static_cast<int32_t>(offsetX), static_cast<int32_t>(offsetY),
        static_cast<int32_t>(width), static_cast<int32_t>(height));

    return { width, height };
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

        GLFWmonitor*       monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode    = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(glfwWindow, monitor, 0, 0, mode->width,
                             mode->height, mode->refreshRate);
    } else {
        if (prevW <= 0) {
            prevW = defaultWidth;
        }
        if (prevH <= 0) {
            prevH = defaultHeight;
        }

        GLFWmonitor*       monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode    = glfwGetVideoMode(monitor);

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
    glfwSetCursorPos(glfwWindow, window->getWidth() / 2.F,
                     window->getHeight() / 2.F);
    Input::setPrevCursorPos(
        { window->getWidth() / 2.F, window->getHeight() / 2.F });
}

void Application::setVerticalSync(const bool val) {
    vsync = val;
    glfwSwapInterval(vsync ? 1 : 0);
    SPONGE_CORE_DEBUG(fmt::format("Set vsync to {}", vsync));
}

void Application::run() {
#ifdef ENABLE_PROFILING
    SPONGE_CORE_DEBUG("Tracy profiler enabled");
#endif

    SPONGE_CORE_INFO("Starting");
    if (!start()) {
        return;
    }

    SPONGE_CORE_INFO("Iterating loop");
    bool quit = false;
    while (!quit) {
        quit = iterateLoop();
    }

    SPONGE_CORE_INFO("Shutting down");
    shutdown();
}
}  // namespace sponge::platform::glfw::core
