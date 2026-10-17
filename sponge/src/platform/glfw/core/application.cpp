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
#include <utility>
#include <vector>

namespace {
sponge::core::Timer mainTimer;

// A minimised window does no update and no render, and the loop wakes this
// often to pump events. An unfocused window and a menu still run, at no more
// than 30 frames a second. Neither value is a setting, so raise them only if a
// case needs it.
constexpr double minimizedWaitSeconds = 0.1;
constexpr auto   oneSecond = std::chrono::nanoseconds(std::chrono::seconds(1));
constexpr auto   slowFrameInterval = oneSecond / 30;

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

// Both null checks fail together in practice, because a missing monitor has no
// mode, but GLFW exposes them as two separate calls.
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

void enterFullScreen(GLFWwindow* window, const MonitorMode& mm) {
    int curW = 0;
    int curH = 0;
    glfwGetWindowSize(window, &curW, &curH);
    glfwSetWindowMonitor(window, mm.monitor, 0, 0, curW, curH,
                         mm.mode->refreshRate);
}

void enterBorderless(GLFWwindow* window, const MonitorMode& mm) {
    int monitorX = 0;
    int monitorY = 0;
    glfwGetMonitorPos(mm.monitor, &monitorX, &monitorY);
    glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);
    glfwSetWindowMonitor(window, nullptr, monitorX, monitorY, mm.mode->width,
                         mm.mode->height, GLFW_DONT_CARE);
    // Leaving full screen restores the desktop mode, which may differ from the
    // full screen mode read above.
    if (const auto* desktop = glfwGetVideoMode(mm.monitor)) {
        glfwSetWindowSize(window, desktop->width, desktop->height);
    }
}

// windowedW and windowedH are the size to restore. A size that is not positive
// falls back to the spec, and then to the current size.
void enterWindowed(GLFWwindow* window, const MonitorMode& mm,
                   const uint32_t specW, const uint32_t specH,
                   int32_t& windowedW, int32_t& windowedH) {
    if (windowedW <= 0) {
        windowedW = static_cast<int>(specW);
    }
    if (windowedH <= 0) {
        windowedH = static_cast<int>(specH);
    }
    // A pristine settings file gives a 0x0 spec, and GLFW rejects a 0x0 window
    // without changing mode, so this keeps the current size and a pending
    // resolution resizes it later.
    if (windowedW <= 0 || windowedH <= 0) {
        glfwGetWindowSize(window, &windowedW, &windowedH);
    }
    glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_TRUE);
    glfwSetWindowMonitor(window, nullptr, 0, 0, windowedW, windowedH,
                         GLFW_DONT_CARE);
    // Centre on the desktop mode read after the call, because leaving full
    // screen restores it and it may differ from the full screen mode in mm.
    if (const auto* desktop = glfwGetVideoMode(mm.monitor)) {
        glfwSetWindowPos(window, (desktop->width - windowedW) / 2,
                         (desktop->height - windowedH) / 2);
    }
    glfwFocusWindow(window);
}
}  // namespace

namespace sponge::platform::glfw::core {
using audio::Audio;
using opengl::debug::Diagnostics;
using opengl::renderer::Context;
using opengl::renderer::RendererAPI;

namespace {
// GLFW otherwise fails silently, for example when glfwSetWindowMonitor rejects
// a 0x0 size. The callback is set before glfwInit so that init errors are
// logged too.
bool initGlfw() {
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
    return true;
}
}  // namespace

Application* Application::instance = nullptr;

Application::Application(ApplicationSpecification specification) :
    appSpec(std::move(specification)) {
    frameRateCap.store(appSpec.frameRateCap, std::memory_order_relaxed);
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

    if (!initGlfw()) {
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

    // Only update-thread layers run here. Render-thread layers run in the
    // render task.
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
    // vsync changes at once, so hasVerticalSync() is correct for the caller,
    // and only glfwSwapInterval is deferred to the thread that owns the
    // context.
    vsync = val;
    pendingVerticalSyncValue.store(val, std::memory_order_relaxed);
    pendingVerticalSync.store(true, std::memory_order_release);
}

void Application::setResolution(const uint32_t width, const uint32_t height) {
    // Queue the change for the main thread, because GLFW window calls must come
    // from the thread that created the window.
    pendingResolutionW.store(width, std::memory_order_relaxed);
    pendingResolutionH.store(height, std::memory_order_relaxed);
    pendingResolution.store(true, std::memory_order_release);
}

std::vector<sponge::core::Resolution> Application::getAvailableResolutions() {
    return Window::getAvailableResolutions();
}

// Runs on the render thread. The GL context is taken on the first call.
bool Application::renderFrame() {
    auto* glfwWindow = static_cast<GLFWwindow*>(window->getNativeWindow());
    if (!renderContextAcquired) {
        graphics->makeCurrent(glfwWindow);
        // Tracy keeps the GPU context per thread, so create it on the
        // thread that owns the GL context and issues the GPU zones.
        SPONGE_PROFILE_GPU_CONTEXT;
        renderContextAcquired = true;
    }

    const auto taskStart = std::chrono::steady_clock::now();

    // The main thread stored this before kick(), and the worker's condition
    // variable makes the value visible here.
    const double elapsed = renderElapsedTime;

    SPONGE_PROFILE_SECTION("RenderThread:frame");

    applyRenderThreadRequests();
    drawLayers(elapsed);

    const auto flipStart = std::chrono::steady_clock::now();
    {
        SPONGE_PROFILE_SECTION("flip");
        graphics->flip(window->getNativeWindow());
    }
    loopTimes.flipUs.store(microsSince(flipStart), std::memory_order_relaxed);
    loopTimes.renderTaskUs.store(microsSince(taskStart),
                                 std::memory_order_relaxed);
    return true;
}

void Application::applyRenderThreadRequests() {
    if (pendingViewport.load(std::memory_order_acquire)) {
        RendererAPI::setViewport(
            0, 0, pendingViewportW.load(std::memory_order_relaxed),
            pendingViewportH.load(std::memory_order_relaxed));
        pendingViewport.store(false, std::memory_order_relaxed);
    }

    // glfwSwapInterval acts on the current context, which lives on this render
    // thread (released from main in run()) and not on the main thread.
    if (pendingVerticalSync.load(std::memory_order_acquire)) {
        setVerticalSync(
            pendingVerticalSyncValue.load(std::memory_order_relaxed));
        pendingVerticalSync.store(false, std::memory_order_relaxed);
    }
}

void Application::drawLayers(const double elapsed) {
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
        } else if (!layer->onUpdate(elapsed)) {
            renderThreadQuit.store(true, std::memory_order_release);
        }
    }
    {
        SPONGE_PROFILE_SECTION("imgui render");
        imguiManager.end();
    }
}

// Frame 0 runs synchronously to give the render thread a valid snapshot.
void Application::primeFrame() {
    mainTimer.tick();
    glfwPollEvents();
    inputManager.update();
    const double primeElapsed = mainTimer.getElapsedSeconds();
    onUserUpdate(primeElapsed);
    renderElapsedTime = primeElapsed;
}

// Both workers are idle here. Publish the update-thread layer state for the
// next render so that the frame pairing is deterministic.
void Application::frameSync() const {
    for (const auto& layer : *layerStack) {
        if (layer->isActive() && layer->runsOnUpdateThread()) {
            layer->onFrameSync();
        }
    }
}

// glfwSetWindowMonitor deadlocks on Windows when it is called from the render
// or update thread, because the Win32 message pump runs only on the main
// thread. The display mode runs before the resolution so that the display mode
// matches the actual OS state when the resize branch runs.
void Application::applyPendingWindowRequests() {
    if (pendingDisplayMode.load(std::memory_order_acquire)) {
        applyDisplayMode();
        pendingDisplayMode.store(false, std::memory_order_relaxed);
    }

    if (pendingResolution.load(std::memory_order_acquire)) {
        applyResolution(pendingResolutionW.load(std::memory_order_relaxed),
                        pendingResolutionH.load(std::memory_order_relaxed));
        pendingResolution.store(false, std::memory_order_relaxed);
    }

    if (pendingMouseVisible.load(std::memory_order_acquire)) {
        setMouseVisible(
            pendingMouseVisibleValue.load(std::memory_order_relaxed));
        pendingMouseVisible.store(false, std::memory_order_relaxed);
    }
}

void Application::applyDisplayMode() {
    using sponge::core::DisplayMode;
    auto* glfwWindow = static_cast<GLFWwindow*>(window->getNativeWindow());
    if (glfwWindow == nullptr) {
        return;
    }
    const auto mm = getPrimaryMonitorMode();
    if (!mm) {
        return;
    }

    // Remember the windowed rect only when leaving it, so a hop between the
    // other two modes keeps it.
    if (appliedDisplayMode == DisplayMode::Windowed &&
        displayMode != DisplayMode::Windowed) {
        glfwGetWindowSize(glfwWindow, &prevW, &prevH);
    }
    if (displayMode == DisplayMode::FullScreen) {
        enterFullScreen(glfwWindow, *mm);
    } else if (displayMode == DisplayMode::Borderless) {
        enterBorderless(glfwWindow, *mm);
    } else {
        // From full screen the window takes the resolution in use, not the
        // windowed size from before it.
        if (appliedDisplayMode == DisplayMode::FullScreen) {
            glfwGetWindowSize(glfwWindow, &prevW, &prevH);
        }
        enterWindowed(glfwWindow, *mm, appSpec.width, appSpec.height, prevW,
                      prevH);
    }
    appliedDisplayMode = displayMode;
}

// Runs after the display mode so the mode matches OS state. A borderless window
// always keeps the desktop size.
void Application::applyResolution(const uint32_t width, const uint32_t height) {
    auto* glfwWindow = static_cast<GLFWwindow*>(window->getNativeWindow());
    if (glfwWindow == nullptr) {
        return;
    }
    const auto mm = getPrimaryMonitorMode();
    if (!mm) {
        return;
    }

    const auto w = static_cast<int>(width);
    const auto h = static_cast<int>(height);
    if (displayMode == sponge::core::DisplayMode::FullScreen) {
        glfwSetWindowMonitor(glfwWindow, mm->monitor, 0, 0, w, h,
                             mm->mode->refreshRate);
    } else if (displayMode == sponge::core::DisplayMode::Windowed) {
        glfwSetWindowSize(glfwWindow, w, h);
        glfwSetWindowPos(glfwWindow, (mm->mode->width - w) / 2,
                         (mm->mode->height - h) / 2);
    }
}

// The frame interval is the player's cap. While a menu is open or the window is
// unfocused it is at least 1/30 of a second, which also stops vsync off or an
// unpaced swap from spinning the loop. The sleep counts towards the next tick,
// so game time does not slow down.
void Application::limitFrameRate(
    GLFWwindow*                            glfwWindow,
    std::chrono::steady_clock::time_point& lastFrameStart) const {
    std::chrono::nanoseconds interval{ 0 };
    if (const auto cap = frameRateCap.load(std::memory_order_relaxed);
        cap > 0) {
        interval = oneSecond / cap;
    }
    const bool menuOpen =
        std::ranges::any_of(*layerStack, [](const auto& layer) {
            return layer->isActive() && layer->isMenu();
        });
    if (menuOpen ||
        (appSpec.throttleInBackground &&
         glfwGetWindowAttrib(glfwWindow, GLFW_FOCUSED) == GLFW_FALSE)) {
        interval = std::max(interval, slowFrameInterval);
    }
    if (interval.count() <= 0) {
        lastFrameStart = std::chrono::steady_clock::now();
        return;
    }
    const auto target = lastFrameStart + interval;
    sponge::core::Timer::sleepUntil(target);
    // Keep to the schedule, so a late wake does not lengthen every frame after
    // it. After a stall, start again from now, because running frames back to
    // back to catch up would not be a cap at all.
    const auto now = std::chrono::steady_clock::now();
    lastFrameStart = now - target > interval ? now : target;
}

void Application::stopThreads(GLFWwindow* glfwWindow) {
    // The loop exits after it waited for render and before it kicked render
    // again, so render is idle and this returns at once. It stays so that
    // stop() remains safe if the loop gains an exit after the render kick.
    renderThread.waitForComplete();

    // Only the thread that owns the context can release it. A context left
    // current in a finished thread makes the reclaim below fail with
    // BadAccess on Mesa GLX.
    renderThread.kick([this, glfwWindow] {
        graphics->release(glfwWindow);
        return true;
    });
    renderThread.waitForComplete();

    renderThread.stop();
    updateThread.stop();
}

void Application::startThreads(GLFWwindow* glfwWindow) {
    // Transfer GL context to render thread, which acquires it on first wake.
    graphics->release(glfwWindow);

    renderThread.start();
    updateThread.start();

    SPONGE_CORE_INFO("Iterating loop (threaded: 1 update + 1 render)");

    primeFrame();
    frameSync();

    // Warm up the render thread, which acquires the GL context, before the main
    // loop.
    renderThread.kick([this] { return renderFrame(); });
    renderThread.waitForComplete();
}

bool Application::runFrame(
    GLFWwindow*                            glfwWindow,
    std::chrono::steady_clock::time_point& lastFrameStart) {
    mainTimer.tick();
    // Must finish before polling, because GLFW callbacks write ImGui's
    // InputEventsQueue and the render thread reads it.
    const auto renderWaitStart = std::chrono::steady_clock::now();
    renderThread.waitForComplete();
    loopTimes.waitRenderUs.store(microsSince(renderWaitStart),
                                 std::memory_order_relaxed);
    inputManager.recenterCursor();
    glfwPollEvents();
    applyPendingWindowRequests();

    // Wait for update thread before writing new snapshot data.
    const auto updateWaitStart = std::chrono::steady_clock::now();
    const bool updateResult    = updateThread.waitForComplete();
    loopTimes.waitUpdateUs.store(microsSince(updateWaitStart),
                                 std::memory_order_relaxed);

    // Now safe: all previous-frame reads of the snapshot are complete.
    inputManager.update();
    const double elapsed = mainTimer.getElapsedSeconds();

    // A render-thread layer can ask to quit.
    if (!updateResult || renderThreadQuit.load(std::memory_order_acquire)) {
        onUserDestroy();
        return false;
    }

    // Minimised. There is nothing to show, so there is no update, no render
    // and no spinning, because the swap does not block for a minimised
    // window. Both workers are idle here, so skipping the kicks keeps the
    // frame pairing intact. The timer ticks at the top of the loop, so the
    // first frame after a restore sees the last wait and not the whole time
    // minimised. Audio runs on its own device thread and keeps playing.
    if (glfwGetWindowAttrib(glfwWindow, GLFW_ICONIFIED) == GLFW_TRUE) {
        glfwWaitEventsTimeout(minimizedWaitSeconds);
        // The update step that reads a close request does not run, so look at
        // GLFW's own flag, which a close sets before its callback.
        const bool closing = glfwWindowShouldClose(glfwWindow) == GLFW_TRUE;
        if (closing) {
            onUserDestroy();
        }
        return !closing;
    }

    // Update elapsed time for render-thread layers.
    renderElapsedTime = elapsed;

    frameSync();
    limitFrameRate(glfwWindow, lastFrameStart);

    // Game logic for frame N into the free snapshot slot.
    updateThread.kick([this, elapsed] {
        const auto start  = std::chrono::steady_clock::now();
        const bool result = onUserUpdate(elapsed);
        loopTimes.updateTaskUs.store(microsSince(start),
                                     std::memory_order_relaxed);
        return result;
    });

    // GPU work for frame N, which overlaps with update[N].
    renderThread.kick([this] { return renderFrame(); });
    return true;
}

void Application::run() {
#ifdef ENABLE_PROFILING
    SPONGE_CORE_DEBUG("Tracy profiler enabled");
#endif

    SPONGE_CORE_INFO("Starting");
    if (!start()) {
        return;
    }

    auto* glfwWin = static_cast<GLFWwindow*>(window->getNativeWindow());
    startThreads(glfwWin);

    auto lastFrameStart = std::chrono::steady_clock::now();

    // Main loop: update[N] runs at the same time as render[N].
    while (runFrame(glfwWin, lastFrameStart)) {
    }

    stopThreads(glfwWin);

    // Reclaim GL context for shutdown.
    graphics->makeCurrent(glfwWin);

    SPONGE_CORE_INFO("Shutting down");
    shutdown();
}
}  // namespace sponge::platform::glfw::core
