#include "platform/glfw/core/application.hpp"

#include "core/timer.hpp"
#include "debug/profiler.hpp"
#include "event/applicationevent.hpp"
#include "logging/log.hpp"
#include "platform/audio/audio.hpp"
#include "platform/glfw/logging/sink.hpp"
#include "platform/opengl/debug/diagnostics.hpp"
#include "platform/opengl/debug/profiler.hpp"
#include "platform/opengl/renderer/context.hpp"
#include "platform/opengl/renderer/rendererapi.hpp"

#include <GLFW/glfw3.h>
#include <fmt/format.h>
#include <glm/glm.hpp>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {
sponge::core::Timer mainTimer;

// A minimised window does no update and no render; the loop wakes this often
// to pump events. An unfocused window still runs, at no more than 30 frames a
// second. Neither is a setting: raise them only if a case needs it.
constexpr double minimizedWaitSeconds = 0.1;
constexpr auto   backgroundFrameInterval =
    std::chrono::duration<double>(1.0 / 30.0);

uint32_t microsSince(const std::chrono::steady_clock::time_point start) {
    return static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - start)
            .count());
}

struct MonitorMode {
    GLFWmonitor*       monitor;
    const GLFWvidmode* mode;
};

// Both null checks fail together in practice (no monitor -> no mode), but
// GLFW's API exposes them as two separate calls.
std::optional<MonitorMode> getPrimaryMonitorMode() {
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (monitor == nullptr) {
        return std::nullopt;
    }
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    if (mode == nullptr) {
        return std::nullopt;
    }
    return MonitorMode{ monitor, mode };
}
}  // namespace

namespace sponge::platform::glfw::core {
using audio::Audio;
using opengl::debug::Diagnostics;
using opengl::renderer::Context;
using opengl::renderer::RendererAPI;

Application* Application::instance = nullptr;

Application::Application(ApplicationSpecification specification) :
    appSpec(std::move(specification)) {
    const auto guiSink = std::make_shared<imgui::Sink<std::mutex>>();
    logging::Log::addSink(guiSink, logging::Log::fileFormatPattern);

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

    // GLFW otherwise fails silently, e.g. glfwSetWindowMonitor rejecting a
    // 0x0 size. Set before glfwInit so init errors are logged too.
    glfwSetErrorCallback([](const int code, const char* description) {
        SPONGE_CORE_ERROR("GLFW error {:#x}: {}", code, description);
    });

    if (glfwInit() == GLFW_FALSE) {
        const char* descCStr = nullptr;
        glfwGetError(&descCStr);
        const std::string description = descCStr ? descCStr : "Unknown error";
        SPONGE_CORE_CRITICAL(
            fmt::format("Unable to initialize glfw: {}", description));
        return false;
    }

    displayMode        = appSpec.displayMode;
    appliedDisplayMode = displayMode;

    graphics = std::make_unique<Context>();

    window           = std::make_unique<Window>(sponge::core::WindowProps{
        .title       = appName,
        .width       = appSpec.width,
        .height      = appSpec.height,
        .displayMode = appSpec.displayMode,
        .decorated   = appSpec.decorated,
    });
    auto* glfwWindow = static_cast<GLFWwindow*>(window->getNativeWindow());
    if (glfwWindow == nullptr) {
        SPONGE_CORE_CRITICAL("Failed to create window");
        return false;
    }

    graphics->init(glfwWindow);

    Diagnostics::log();

    inputManager.onAttach(glfwWindow);
    imguiManager.onAttach(static_cast<GLFWwindow*>(window->getNativeWindow()));

    Audio::init();

    setVerticalSync(appSpec.vsync);

    renderer = std::make_unique<RendererAPI>();
    renderer->init();
    renderer->setClearColor(glm::vec4{ 0.36F, 0.36F, 0.36F, 1.0F });

    window->setEventCallback([this](event::Event& e) { onEvent(e); });

    const auto w = window->getWidth();
    const auto h = window->getHeight();
    RendererAPI::setViewport(0, 0, static_cast<int32_t>(w),
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
    imguiManager.onDetach();
    Audio::shutdown();
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
        if (!layer->isActive() || !layer->runsOnUpdateThread()) {
            continue;
        }

        if (!layer->onUpdate(elapsedTime)) {
            result = false;
            break;
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

void Application::setDisplayMode(const sponge::core::DisplayMode mode) {
    displayMode = mode;
    pendingDisplayMode.store(true, std::memory_order_release);
}

void Application::setMouseVisible(const bool value) const {
    auto* glfwWindow = static_cast<GLFWwindow*>(window->getNativeWindow());
    if (value) {
        glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    } else {
        glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
}

void Application::requestMouseVisible(const bool value) {
    pendingMouseVisibleValue.store(value, std::memory_order_relaxed);
    pendingMouseVisible.store(true, std::memory_order_release);
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
}

void Application::requestVerticalSync(const bool val) {
    // vsync is authoritative immediately so hasVerticalSync() is truthful to
    // the caller; only glfwSwapInterval is deferred to the context owner.
    vsync = val;
    pendingVerticalSyncValue.store(val, std::memory_order_relaxed);
    pendingVerticalSync.store(true, std::memory_order_release);
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

    const std::function<bool()> renderTask = [this, glfwWin,
                                              &renderContextAcquired] {
        if (!renderContextAcquired) {
            graphics->makeCurrent(glfwWin);
            // Tracy keeps the GPU context per thread, so create it on the
            // thread that owns the GL context and issues the GPU zones.
            SPONGE_PROFILE_GPU_CONTEXT;
            renderContextAcquired = true;
        }

        const auto taskStart = std::chrono::steady_clock::now();

        // Visible via cond-var acquire fence; main thread stored before kick().
        const double elapsed = renderElapsedTime;

        SPONGE_PROFILE_SECTION("RenderThread:frame");

        if (pendingViewport.load(std::memory_order_acquire)) {
            RendererAPI::setViewport(
                0, 0, pendingViewportW.load(std::memory_order_relaxed),
                pendingViewportH.load(std::memory_order_relaxed));
            pendingViewport.store(false, std::memory_order_relaxed);
        }

        // glfwSwapInterval acts on the current context, which lives on this
        // render thread (released from main above) — not the main thread.
        if (pendingVerticalSync.load(std::memory_order_acquire)) {
            setVerticalSync(
                pendingVerticalSyncValue.load(std::memory_order_relaxed));
            pendingVerticalSync.store(false, std::memory_order_relaxed);
        }

        {
            SPONGE_PROFILE_SECTION("clear");
            const bool covered =
                std::ranges::any_of(*layerStack, [](const auto& layer) {
                    return layer->isActive() && layer->coversScreen();
                });
            renderer->clear(!covered);
        }
        {
            SPONGE_PROFILE_SECTION("imgui build");
            imguiManager.begin();
            onImGuiRender();
        }
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
        {
            SPONGE_PROFILE_SECTION("imgui render");
            imguiManager.end();
        }
        const auto flipStart = std::chrono::steady_clock::now();
        {
            SPONGE_PROFILE_SECTION("flip");
            graphics->flip(window->getNativeWindow());
        }
        loopTimes.flipUs.store(microsSince(flipStart),
                               std::memory_order_relaxed);
        loopTimes.renderTaskUs.store(microsSince(taskStart),
                                     std::memory_order_relaxed);
        return true;
    };

    renderThread.start();
    updateThread.start();

    SPONGE_CORE_INFO("Iterating loop (threaded: 1 update + 1 render)");

    // Prime: frame 0 runs synchronously to give render thread a valid snapshot.
    mainTimer.tick();
    glfwPollEvents();
    inputManager.update();
    {
        const double primeElapsed = mainTimer.getElapsedSeconds();
        onUserUpdate(primeElapsed);
        renderElapsedTime = primeElapsed;
    }

    // Both workers idle: publish update-thread layer state for the next
    // render so the frame pairing is deterministic.
    const auto frameSync = [this] {
        for (const auto& layer : *layerStack) {
            if (layer->isActive() && layer->runsOnUpdateThread()) {
                layer->onFrameSync();
            }
        }
    };

    frameSync();

    // Warm up render thread (acquires GL context) before pipelined loop.
    renderThread.kick(renderTask);
    renderThread.waitForComplete();

    auto lastFrameStart = std::chrono::steady_clock::now();

    // Pipelined loop: update[N] overlaps with render[N].
    while (true) {
        mainTimer.tick();
        // Must finish before poll: GLFW callbacks write ImGui's
        // InputEventsQueue; render thread reads it.
        const auto renderWaitStart = std::chrono::steady_clock::now();
        renderThread.waitForComplete();
        loopTimes.waitRenderUs.store(microsSince(renderWaitStart),
                                     std::memory_order_relaxed);
        inputManager.recenterCursor();
        glfwPollEvents();

        // glfwSetWindowMonitor from render/update threads deadlocks on Windows
        // (Win32 message pump runs only here). Runs before pendingResolution
        // so the display mode matches the actual OS state when the resize
        // branch fires.
        if (pendingDisplayMode.load(std::memory_order_acquire)) {
            using sponge::core::DisplayMode;
            auto* glfwWindow =
                static_cast<GLFWwindow*>(window->getNativeWindow());
            if (glfwWindow != nullptr) {
                if (const auto mm = getPrimaryMonitorMode()) {
                    // Remember the windowed rect only when leaving it, so a
                    // hop between the other two modes keeps it.
                    if (appliedDisplayMode == DisplayMode::Windowed &&
                        displayMode != DisplayMode::Windowed) {
                        glfwGetWindowPos(glfwWindow, &prevX, &prevY);
                        glfwGetWindowSize(glfwWindow, &prevW, &prevH);
                    }
                    if (displayMode == DisplayMode::FullScreen) {
                        int curW = 0;
                        int curH = 0;
                        glfwGetWindowSize(glfwWindow, &curW, &curH);
                        glfwSetWindowMonitor(glfwWindow, mm->monitor, 0, 0,
                                             curW, curH, mm->mode->refreshRate);
                    } else if (displayMode == DisplayMode::Borderless) {
                        int monitorX = 0;
                        int monitorY = 0;
                        glfwGetMonitorPos(mm->monitor, &monitorX, &monitorY);
                        glfwSetWindowAttrib(glfwWindow, GLFW_DECORATED,
                                            GLFW_FALSE);
                        glfwSetWindowMonitor(glfwWindow, nullptr, monitorX,
                                             monitorY, mm->mode->width,
                                             mm->mode->height, GLFW_DONT_CARE);
                        // Leaving full screen restores the desktop mode, which
                        // may differ from the full screen mode read above.
                        if (const auto* desktop =
                                glfwGetVideoMode(mm->monitor)) {
                            glfwSetWindowSize(glfwWindow, desktop->width,
                                              desktop->height);
                        }
                    } else {
                        // From full screen the window takes the resolution in
                        // use, not the windowed size from before it.
                        if (appliedDisplayMode == DisplayMode::FullScreen) {
                            glfwGetWindowSize(glfwWindow, &prevW, &prevH);
                        }
                        if (prevW <= 0) {
                            prevW = static_cast<int>(appSpec.width);
                        }
                        if (prevH <= 0) {
                            prevH = static_cast<int>(appSpec.height);
                        }
                        // A pristine settings file gives a 0x0 spec, and
                        // GLFW rejects a 0x0 window without changing
                        // mode. Keep the current size; a pending
                        // resolution resizes it below.
                        if (prevW <= 0 || prevH <= 0) {
                            glfwGetWindowSize(glfwWindow, &prevW, &prevH);
                        }
                        glfwSetWindowAttrib(glfwWindow, GLFW_DECORATED,
                                            GLFW_TRUE);
                        glfwSetWindowMonitor(glfwWindow, nullptr, 0, 0, prevW,
                                             prevH, GLFW_DONT_CARE);
                        // Centre on the desktop mode read after the call:
                        // leaving full screen restores it, and it may differ
                        // from the full screen mode in mm.
                        if (const auto* desktop =
                                glfwGetVideoMode(mm->monitor)) {
                            glfwSetWindowPos(glfwWindow,
                                             (desktop->width - prevW) / 2,
                                             (desktop->height - prevH) / 2);
                        }
                        glfwFocusWindow(glfwWindow);
                    }
                    appliedDisplayMode = displayMode;
                }
            }
            pendingDisplayMode.store(false, std::memory_order_relaxed);
        }

        // Runs after pendingDisplayMode so the mode matches OS state. A
        // borderless window always keeps the desktop size.
        if (pendingResolution.load(std::memory_order_acquire)) {
            const uint32_t w =
                pendingResolutionW.load(std::memory_order_relaxed);
            const uint32_t h =
                pendingResolutionH.load(std::memory_order_relaxed);
            auto* glfwWindow =
                static_cast<GLFWwindow*>(window->getNativeWindow());
            if (glfwWindow != nullptr) {
                if (const auto mm = getPrimaryMonitorMode()) {
                    if (displayMode == sponge::core::DisplayMode::FullScreen) {
                        glfwSetWindowMonitor(
                            glfwWindow, mm->monitor, 0, 0, static_cast<int>(w),
                            static_cast<int>(h), mm->mode->refreshRate);
                    } else if (displayMode ==
                               sponge::core::DisplayMode::Windowed) {
                        glfwSetWindowSize(glfwWindow, static_cast<int>(w),
                                          static_cast<int>(h));
                        const int posX =
                            (mm->mode->width - static_cast<int>(w)) / 2;
                        const int posY =
                            (mm->mode->height - static_cast<int>(h)) / 2;
                        glfwSetWindowPos(glfwWindow, posX, posY);
                    }
                }
            }
            pendingResolution.store(false, std::memory_order_relaxed);
        }

        if (pendingMouseVisible.load(std::memory_order_acquire)) {
            setMouseVisible(
                pendingMouseVisibleValue.load(std::memory_order_relaxed));
            pendingMouseVisible.store(false, std::memory_order_relaxed);
        }

        // Wait for update thread before writing new snapshot data.
        const auto updateWaitStart = std::chrono::steady_clock::now();
        const bool updateResult    = updateThread.waitForComplete();
        loopTimes.waitUpdateUs.store(microsSince(updateWaitStart),
                                     std::memory_order_relaxed);

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

        // Minimised: nothing to show, so no update, no render and no spinning
        // (the swap does not block for a minimised window). Both workers are
        // idle here, so skipping the kicks keeps the frame pairing intact.
        // The timer ticks at the top of the loop, so the first frame after a
        // restore sees the last wait, not the whole time minimised. Audio runs
        // on its own device thread and keeps playing.
        if (glfwGetWindowAttrib(glfwWin, GLFW_ICONIFIED) == GLFW_TRUE) {
            glfwWaitEventsTimeout(minimizedWaitSeconds);
            // The update step that reads a close request does not run, so
            // look at GLFW's own flag, which a close sets before its callback.
            if (glfwWindowShouldClose(glfwWin) == GLFW_TRUE) {
                onUserDestroy();
                break;
            }
            continue;
        }

        // Update elapsed time for render-thread layers.
        renderElapsedTime = elapsed;

        frameSync();

        // Unfocused: cap the rate, so vsync off or an unpaced swap cannot spin
        // the loop. The sleep counts towards the next tick, so game time does
        // not slow down.
        if (appSpec.throttleInBackground &&
            glfwGetWindowAttrib(glfwWin, GLFW_FOCUSED) == GLFW_FALSE) {
            std::this_thread::sleep_until(lastFrameStart +
                                          backgroundFrameInterval);
        }
        lastFrameStart = std::chrono::steady_clock::now();

        // Game logic for frame N into the free snapshot slot.
        updateThread.kick([this, elapsed] {
            const auto start  = std::chrono::steady_clock::now();
            const bool result = onUserUpdate(elapsed);
            loopTimes.updateTaskUs.store(microsSince(start),
                                         std::memory_order_relaxed);
            return result;
        });

        // GPU work for frame N; overlaps with update[N].
        renderThread.kick(renderTask);
    }

    // The only exit is the break above, after this pass already waited for
    // render and before it kicked render again, so render is idle and this
    // returns at once. Kept so stop() stays safe if the loop gains an exit
    // after the render kick.
    renderThread.waitForComplete();

    // Only the thread that owns the context can release it. A context left
    // current in a finished thread makes the reclaim below fail with
    // BadAccess on Mesa GLX.
    renderThread.kick([this, glfwWin] {
        graphics->release(glfwWin);
        return true;
    });
    renderThread.waitForComplete();

    renderThread.stop();
    updateThread.stop();

    // Reclaim GL context for shutdown.
    graphics->makeCurrent(glfwWin);

    SPONGE_CORE_INFO("Shutting down");
    shutdown();
}
}  // namespace sponge::platform::glfw::core
