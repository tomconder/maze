#pragma once

#include "layer/layer.hpp"
#include "layer/layerstack.hpp"
#include "logging/log.hpp"
#include "platform/glfw/core/inputmanager.hpp"
#include "platform/glfw/core/window.hpp"
#include "platform/glfw/imgui/glfwmanager.hpp"
#include "platform/glfw/imgui/noopmanager.hpp"
#include "platform/opengl/renderer/context.hpp"
#include "platform/opengl/renderer/rendererapi.hpp"
#include "thread/worker.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace sponge::platform::glfw::core {
struct ApplicationSpecification {
    std::string_view          name   = "Sponge";
    uint32_t                  width  = 1600;
    uint32_t                  height = 900;
    sponge::core::DisplayMode displayMode =
        sponge::core::DisplayMode::FullScreen;
    bool vsync     = true;
    bool decorated = true;
    // Cap the frame rate while the window is unfocused. A capture run turns
    // it off so its timings do not depend on which window has focus.
    bool throttleInBackground = true;
    // Most frames a second the loop runs, where 0 means no cap.
    uint32_t frameRateCap = 0;
};

using logging::LogItem;

class Application {
public:
    explicit Application(ApplicationSpecification specification);

    virtual ~Application();

    virtual bool start();

    virtual void shutdown();

    virtual bool onUserCreate();

    virtual bool onUserUpdate(double elapsedTime);

    virtual bool onUserDestroy();

    virtual void onEvent(event::Event& event);

    void onImGuiRender() const;

    void pushOverlay(const std::shared_ptr<layer::Layer>& layer) const;

    void pushLayer(const std::shared_ptr<layer::Layer>& layer) const;

    void setDisplayMode(sponge::core::DisplayMode mode);

    void setResolution(uint32_t width, uint32_t height);

    static std::vector<sponge::core::Resolution> getAvailableResolutions();

    sponge::core::DisplayMode getDisplayMode() const {
        return displayMode;
    }

    layer::LayerStack* getLayerStack() const {
        return layerStack.get();
    }

    bool hasVerticalSync() const {
        return vsync;
    }

    void setVerticalSync(bool val);

    // Thread-safe request to change vsync from a non-main thread (e.g. an
    // update-thread or render-thread layer). hasVerticalSync() reflects the new
    // value at once, and glfwSwapInterval runs on the context-owning thread.
    void requestVerticalSync(bool val);

    // Thread-safe, because the main loop reads it each frame. 0 removes the
    // cap.
    void setFrameRateCap(const uint32_t fps) {
        frameRateCap.store(fps, std::memory_order_relaxed);
    }

    std::vector<LogItem>& getMessages() const {
        return *messages;
    }

    void addMessage(const LogItem& item) const {
        messages->emplace_back(item);
    }

    void setMouseVisible(bool value) const;

    // Thread-safe request to change mouse visibility from a non-main thread
    // (e.g. an update-thread layer). Applied on the main thread, which owns
    // the GLFW window.
    void requestMouseVisible(bool value);

    void centerMouse();

    void setPendingViewport(const int32_t w, const int32_t h) {
        pendingViewportW.store(w, std::memory_order_relaxed);
        pendingViewportH.store(h, std::memory_order_relaxed);
        pendingViewport.store(true, std::memory_order_release);
    }

    static Application& get() {
        return *instance;
    }

    std::shared_ptr<Window> getWindow() const {
        return window;
    }

    InputManager& getInputManager() {
        return inputManager;
    }
    const InputManager& getInputManager() const {
        return inputManager;
    }
    std::pair<float, float> getMousePosition() const;

    virtual void run();

    bool isEventHandledByImGui() const {
        return imguiManager.isEventHandled();
    }

    // Wall time of each part of the frame loop in microseconds, for the last
    // frame that finished it. Each field has one writer thread. Use these to
    // find which thread sets the frame rate.
    struct LoopTimes {
        // Render thread: the whole render task, and the buffer swap in it.
        std::atomic<uint32_t> renderTaskUs{ 0 };
        std::atomic<uint32_t> flipUs{ 0 };
        // Update thread: the whole update task.
        std::atomic<uint32_t> updateTaskUs{ 0 };
        // Main thread: time blocked waiting for the render and update tasks.
        std::atomic<uint32_t> waitRenderUs{ 0 };
        std::atomic<uint32_t> waitUpdateUs{ 0 };
    };
    LoopTimes loopTimes;

private:
    // Render thread.
    bool renderFrame();
    void applyRenderThreadRequests();
    void drawLayers(double elapsed);

    // Main thread, in run().
    void primeFrame();
    void frameSync() const;
    void applyPendingWindowRequests();
    void applyDisplayMode();
    void applyResolution(uint32_t width, uint32_t height);
    void limitFrameRate(
        GLFWwindow*                            glfwWindow,
        std::chrono::steady_clock::time_point& lastFrameStart) const;
    void stopThreads(GLFWwindow* glfwWindow);

    std::string_view                               appName = "undefined";
    std::unique_ptr<opengl::renderer::Context>     graphics;
    std::unique_ptr<opengl::renderer::RendererAPI> renderer;

    std::shared_ptr<Window> window;

    std::unique_ptr<std::vector<LogItem>> messages;

    sponge::core::DisplayMode displayMode = sponge::core::DisplayMode::Windowed;
    // The mode the OS window is in; only the main loop writes it.
    sponge::core::DisplayMode appliedDisplayMode =
        sponge::core::DisplayMode::Windowed;
    bool    vsync = true;
    int32_t prevH = 0;
    int32_t prevW = 0;

    std::unique_ptr<layer::LayerStack> layerStack;

    ApplicationSpecification appSpec;

    // One update thread (game logic, no GL) and one render thread (owns the GL
    // context).
    thread::Worker renderThread;
    thread::Worker updateThread;

    InputManager inputManager;

    // Elapsed time for render-thread layers. The main thread writes it and the
    // render thread reads it.
    double renderElapsedTime{ 0.0 };

    // Render thread only: set once it has taken the GL context.
    bool renderContextAcquired = false;

    // Signals quit from a render-thread layer back to the main loop.
    std::atomic<bool> renderThreadQuit{ false };

    // Deferred viewport update; applied by render thread at start of each
    // frame.
    std::atomic<bool>    pendingViewport{ false };
    std::atomic<int32_t> pendingViewportW{ 0 };
    std::atomic<int32_t> pendingViewportH{ 0 };

    // Deferred resolution change requested from render-thread layers (e.g.
    // OptionLayer). Applied on main thread to keep all GLFW window ops on the
    // thread that owns the window.
    std::atomic<bool>     pendingResolution{ false };
    std::atomic<uint32_t> pendingResolutionW{ 0 };
    std::atomic<uint32_t> pendingResolutionH{ 0 };

    // displayMode changes immediately in setDisplayMode(), and the GLFW window
    // call runs on the main thread to avoid deadlocking the Win32 message pump.
    std::atomic<bool> pendingDisplayMode{ false };

    // Deferred mouse-visibility change requested from update-thread layers
    // (e.g. MazeLayer's pause handling). glfwSetInputMode must run on the
    // thread that owns the window.
    std::atomic<bool> pendingMouseVisible{ false };
    std::atomic<bool> pendingMouseVisibleValue{ false };

    // Deferred vsync change requested from update-thread or render-thread
    // layers (e.g. OptionLayer, ImGuiLayer). glfwSwapInterval must run on
    // the thread that owns the window/context.
    std::atomic<bool> pendingVerticalSync{ false };
    std::atomic<bool> pendingVerticalSyncValue{ false };

    std::atomic<uint32_t> frameRateCap{ 0 };

    static Application* instance;

#ifdef ENABLE_IMGUI
    imgui::GLFWManager imguiManager;
#else
    imgui::NoopManager imguiManager;
#endif
};

// implemented by the client
std::unique_ptr<Application> createApplication(int argc, char** argv);

}  // namespace sponge::platform::glfw::core
