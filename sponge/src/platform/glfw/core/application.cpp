#include "platform/glfw/core/application.hpp"

#include "core/timer.hpp"
#include "debug/profiler.hpp"
#include "event/applicationevent.hpp"
#include "logging/log.hpp"
#include "platform/glfw/logging/sink.hpp"
#include "platform/opengl/debug/diagnostics.hpp"
#include "platform/opengl/renderer/context.hpp"
#include "platform/opengl/renderer/rendererapi.hpp"

#include <fmt/format.h>
#include <glm/glm.hpp>

#include <array>
#include <memory>
#include <utility>

namespace {
sponge::core::Timer mainTimer;
}  // namespace

namespace sponge::platform::glfw::core {
using opengl::debug::Diagnostics;
using opengl::renderer::Context;
using opengl::renderer::RendererAPI;

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

    graphics = std::make_unique<Context>();

    window = std::make_unique<Window>(
        sponge::core::WindowProps{ .title      = appName,
                                   .width      = appSpec.width,
                                   .height     = appSpec.height,
                                   .fullscreen = appSpec.fullscreen });
    auto* glfwWindow = static_cast<GLFWwindow*>(window->getNativeWindow());
    if (glfwWindow == nullptr) {
        SPONGE_CORE_CRITICAL("Failed to create window");
        return false;
    }

    graphics->init(glfwWindow);

    Diagnostics::log();

    inputManager.onAttach(glfwWindow);
    imguiManager->onAttach(static_cast<GLFWwindow*>(window->getNativeWindow()));

    setVerticalSync(appSpec.vsync);

    renderer = std::make_unique<RendererAPI>();
    renderer->init();
    renderer->setClearColor(glm::vec4{ 0.36F, 0.36F, 0.36F, 1.0F });

    window->setEventCallback([this](event::Event& e) { onEvent(e); });

    const auto w = window->getWidth();
    const auto h = window->getHeight();
    renderer->setViewport(0, 0, static_cast<int32_t>(w),
                          static_cast<int32_t>(h));

    if (!onUserCreate()) {
        return false;
    }

    auto resizeEvent = event::WindowResizeEvent{ w, h };
    onEvent(resizeEvent);

    glfwShowWindow(static_cast<GLFWwindow*>(window->getNativeWindow()));

    return true;
}

void Application::shutdown() {
    inputManager.onDetach();
    imguiManager->onDetach();
}

bool Application::onUserCreate() {
    return true;
}

bool Application::onUserUpdate(const double elapsedTime) {
    SPONGE_PROFILE_SECTION("Application::onUserUpdate");

    // Only update-thread layers; render-thread layers run in the render thread
    // lambda.
    bool result = true;

    for (const auto& layer : *layerStack) {
        if (layer->isActive() && layer->runsOnUpdateThread()) {
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

    for (auto it = layerStack->rbegin(); it != layerStack->rend(); ++it) {
        const auto& layer = *it;
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
    fullscreen = !fullscreen;
    pendingFullscreenToggle.store(true, std::memory_order_release);
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

void Application::centerMouse() {
    const auto cx = static_cast<double>(window->getWidth()) / 2.0;
    const auto cy = static_cast<double>(window->getHeight()) / 2.0;
    glfwSetCursorPos(static_cast<GLFWwindow*>(window->getNativeWindow()), cx,
                     cy);
    inputManager.onMouseWarped();
}

std::pair<float, float> Application::getMousePosition() const {
    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(static_cast<GLFWwindow*>(window->getNativeWindow()), &x,
                     &y);
    return { static_cast<float>(x), static_cast<float>(y) };
}

void Application::setVerticalSync(const bool val) {
    vsync = val;
    glfwSwapInterval(vsync ? 1 : 0);
    SPONGE_CORE_DEBUG(fmt::format("Set vsync to {}", vsync));
}

void Application::setResolution(const uint32_t width, const uint32_t height) {
    // Queue for application on main thread (GLFW window calls must originate
    // from the thread that created/owns the window).
    pendingResolutionW.store(width, std::memory_order_relaxed);
    pendingResolutionH.store(height, std::memory_order_relaxed);
    pendingResolution.store(true, std::memory_order_release);
}

std::vector<sponge::core::Resolution> Application::getAvailableResolutions() {
    return Window::getAvailableResolutions();
}

void Application::run() {
#ifdef ENABLE_PROFILING
    SPONGE_CORE_DEBUG("Tracy profiler enabled");
#endif

    SPONGE_CORE_INFO("Starting");
    if (!start()) {
        return;
    }

    // Transfer GL context to render thread.
    auto* glfwWin = static_cast<GLFWwindow*>(window->getNativeWindow());
    graphics->release(glfwWin);

    // Render thread acquires GL context on first wake.
    bool renderContextAcquired = false;
    renderThread.start([this, glfwWin, &renderContextAcquired] {
        if (!renderContextAcquired) {
            graphics->makeCurrent(glfwWin);
            renderContextAcquired = true;
        }

        // Visible via cond-var acquire fence; main thread stored before kick().
        const double elapsed = renderElapsedTime;

        SPONGE_PROFILE_SECTION("RenderThread:frame");

        if (pendingViewport.load(std::memory_order_acquire)) {
            renderer->setViewport(
                0, 0, pendingViewportW.load(std::memory_order_relaxed),
                pendingViewportH.load(std::memory_order_relaxed));
            pendingViewport.store(false, std::memory_order_relaxed);
        }

        renderer->clear();
        imguiManager->begin();
#ifdef ENABLE_IMGUI
        onImGuiRender();
#endif
        for (const auto& layer : *layerStack) {
            if (!layer->isActive()) {
                continue;
            }
            if (layer->runsOnUpdateThread()) {
                layer->onRender();
            } else {
                if (!layer->onUpdate(elapsed)) {
                    renderThreadQuit.store(true, std::memory_order_release);
                }
            }
        }
        imguiManager->end();
        graphics->flip(window->getNativeWindow());
    });

    // Start the two update worker threads.
    updateThreads[0].start();
    updateThreads[1].start();

    SPONGE_CORE_INFO("Iterating loop (threaded: 2 update + 1 render)");

    uint32_t frame = 0;

    // Prime: frame 0 runs synchronously to give render thread a valid snapshot.
    mainTimer.tick();
    glfwPollEvents();
    inputManager.update();
    {
        const double primeElapsed = mainTimer.getElapsedSeconds();
        onUserUpdate(primeElapsed);
        renderElapsedTime = primeElapsed;
    }

    // Warm up render thread (acquires GL context) before pipelined loop.
    renderThread.kick();
    renderThread.blockUntilRenderComplete();
    ++frame;

    // Pipelined loop: update[N] overlaps with render[N-1].
    while (true) {
        const uint32_t updateIdx = frame % 2;

        mainTimer.tick();
        // Must finish before poll: GLFW callbacks write ImGui's
        // InputEventsQueue; render thread reads it.
        renderThread.blockUntilRenderComplete();
        inputManager.recenterCursor();
        glfwPollEvents();

        // glfwSetWindowMonitor from render/update threads deadlocks on Windows
        // (Win32 message pump runs only here). Runs before pendingResolution
        // so fullscreen matches the actual OS state when the resize branch
        // fires.
        if (pendingFullscreenToggle.load(std::memory_order_acquire)) {
            auto* glfwWindow =
                static_cast<GLFWwindow*>(window->getNativeWindow());
            if (glfwWindow != nullptr) {
                GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                if (monitor != nullptr) {
                    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                    if (mode != nullptr) {
                        if (fullscreen) {
                            glfwGetWindowPos(glfwWindow, &prevX, &prevY);
                            glfwGetWindowSize(glfwWindow, &prevW, &prevH);
                            glfwSetWindowMonitor(glfwWindow, monitor, 0, 0,
                                                 prevW, prevH,
                                                 mode->refreshRate);
                        } else {
                            if (prevW <= 0) {
                                prevW = static_cast<int>(appSpec.width);
                            }
                            if (prevH <= 0) {
                                prevH = static_cast<int>(appSpec.height);
                            }
                            const int posX = (mode->width - prevW) / 2;
                            const int posY = (mode->height - prevH) / 2;
                            glfwSetWindowAttrib(glfwWindow, GLFW_DECORATED,
                                                GLFW_TRUE);
                            glfwSetWindowMonitor(glfwWindow, nullptr, posX,
                                                 posY, prevW, prevH,
                                                 GLFW_DONT_CARE);
                            glfwFocusWindow(glfwWindow);
                        }
                    }
                }
            }
            pendingFullscreenToggle.store(false, std::memory_order_relaxed);
        }

        // Runs after pendingFullscreenToggle so fullscreen matches OS state.
        if (pendingResolution.load(std::memory_order_acquire)) {
            const uint32_t w =
                pendingResolutionW.load(std::memory_order_relaxed);
            const uint32_t h =
                pendingResolutionH.load(std::memory_order_relaxed);
            auto* glfwWindow =
                static_cast<GLFWwindow*>(window->getNativeWindow());
            if (glfwWindow != nullptr) {
                GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                if (monitor != nullptr) {
                    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                    if (mode != nullptr) {
                        if (fullscreen) {
                            glfwSetWindowMonitor(
                                glfwWindow, monitor, 0, 0, static_cast<int>(w),
                                static_cast<int>(h), mode->refreshRate);
                        } else {
                            glfwSetWindowSize(glfwWindow, static_cast<int>(w),
                                              static_cast<int>(h));
                            const int posX =
                                (mode->width - static_cast<int>(w)) / 2;
                            const int posY =
                                (mode->height - static_cast<int>(h)) / 2;
                            glfwSetWindowPos(glfwWindow, posX, posY);
                        }
                    }
                }
            }
            pendingResolution.store(false, std::memory_order_relaxed);
        }

        // Wait for update thread before writing new snapshot data.
        const bool updateResult = updateThreads[updateIdx].waitForComplete();

        // Now safe: all previous-frame reads of the snapshot are complete.
        inputManager.update();
        const double elapsed = mainTimer.getElapsedSeconds();

        // Check if a render-thread layer signaled quit.
        const bool renderQuit =
            renderThreadQuit.load(std::memory_order_acquire);

        if (!updateResult || renderQuit) {
            onUserDestroy();
            break;
        }

        // Update elapsed time for render-thread layers.
        renderElapsedTime = elapsed;

        // Game logic for this frame into renderFrames[updateIdx].
        updateThreads[updateIdx].kick(elapsed, [this](const double dt) -> bool {
            return onUserUpdate(dt);
        });

        // GPU work for frame N; overlaps with update[N+1].
        renderThread.kick();

        frame++;
    }

    // Drain in-flight render before stopping threads.
    renderThread.blockUntilRenderComplete();

    renderThread.stop();
    updateThreads[0].stop();
    updateThreads[1].stop();

    // Reclaim GL context for shutdown.
    graphics->makeCurrent(glfwWin);

    SPONGE_CORE_INFO("Shutting down");
    shutdown();
}
}  // namespace sponge::platform::glfw::core
