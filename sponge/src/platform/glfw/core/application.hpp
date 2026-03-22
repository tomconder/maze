#pragma once

#include "core/application.hpp"
#include "layer/layer.hpp"
#include "layer/layerstack.hpp"
#include "logging/log.hpp"
#include "platform/glfw/core/window.hpp"
#include "platform/glfw/imgui/glfwmanager.hpp"
#include "platform/glfw/imgui/noopmanager.hpp"
#include "platform/opengl/renderer/context.hpp"
#include "platform/opengl/renderer/rendererapi.hpp"
#include "thread/renderthread.hpp"
#include "thread/updatethread.hpp"

#include <array>
#include <atomic>
#include <memory>
#include <string_view>
#include <vector>

namespace sponge::platform::glfw::core {
struct ApplicationSpecification {
    std::string_view name       = "Sponge";
    uint32_t         width      = 1600;
    uint32_t         height     = 900;
    bool             fullscreen = true;
    bool             vsync      = true;
};

using logging::LogItem;

class Application : public sponge::core::Application {
public:
    explicit Application(ApplicationSpecification specification);

    ~Application() override;

    bool start() override;

    void shutdown() override;

    bool onUserCreate() override;

    bool onUserUpdate(double elapsedTime) override;

    bool onUserDestroy() override;

    void onEvent(event::Event& event) override;

    void onImGuiRender() const;

    void pushOverlay(const std::shared_ptr<layer::Layer>& layer) const;

    void pushLayer(const std::shared_ptr<layer::Layer>& layer) const;

    void popLayer(const std::shared_ptr<layer::Layer>& layer) const;

    void popOverlay(const std::shared_ptr<layer::Layer>& layer) const;

    void toggleFullscreen();

    void setResolution(uint32_t width, uint32_t height) const;

    std::vector<sponge::core::Resolution> getAvailableResolutions() const;

    bool isFullscreen() const {
        return fullscreen;
    }

    layer::LayerStack* getLayerStack() const {
        return layerStack.get();
    }

    bool hasVerticalSync() const {
        return vsync;
    }

    void setVerticalSync(bool val);

    std::vector<LogItem>& getMessages() const {
        return *messages;
    }

    void addMessage(const LogItem& item) const {
        messages->emplace_back(item);
    }

    void clearMessages() const {
        messages->clear();
    }

    void setMouseVisible(bool value) const;

    void centerMouse() const;

    void setPendingViewport(int32_t w, int32_t h) {
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

    void run() override;

    bool isEventHandledByImGui() const {
        return imguiManager->isEventHandled();
    }

private:
    std::string_view                               appName = "undefined";
    std::unique_ptr<opengl::renderer::Context>     graphics;
    std::unique_ptr<opengl::renderer::RendererAPI> renderer;

    std::shared_ptr<Window> window;

    std::unique_ptr<std::vector<LogItem>> messages;

    bool    fullscreen = false;
    bool    vsync      = true;
    int32_t prevH      = 0;
    int32_t prevW      = 0;
    int32_t prevX      = 0;
    int32_t prevY      = 0;

    std::unique_ptr<layer::LayerStack> layerStack;

    ApplicationSpecification appSpec;

    // 2 update threads (ping-pong, no GL) + 1 render thread (owns GL context).
    sponge::thread::RenderThread                renderThread;
    std::array<sponge::thread::UpdateThread, 2> updateThreads;

    // Elapsed time for render-thread layers; written by main, read by render
    // thread.
    double renderElapsedTime{ 0.0 };

    // Signals quit from a render-thread layer back to the main loop.
    std::atomic<bool> renderThreadQuit{ false };

    // Deferred viewport update; applied by render thread at start of each
    // frame.
    std::atomic<bool>    pendingViewport{ false };
    std::atomic<int32_t> pendingViewportW{ 0 };
    std::atomic<int32_t> pendingViewportH{ 0 };

    static Application* instance;

    std::shared_ptr<imgui::GLFWManager> glfwManager =
        std::make_shared<imgui::GLFWManager>();
    std::shared_ptr<imgui::NoopManager> noopManager =
        std::make_shared<imgui::NoopManager>();

#ifdef ENABLE_IMGUI
    std::shared_ptr<imgui::GLFWManager> imguiManager = glfwManager;
#else
    std::shared_ptr<imgui::NoopManager> imguiManager = noopManager;
#endif
};
}  // namespace sponge::platform::glfw::core
