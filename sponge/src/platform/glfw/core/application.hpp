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
#include <GLFW/glfw3.h>
#include <memory>
#include <string>

namespace sponge::platform::glfw::core {
struct ApplicationSpecification {
    std::string name = "Sponge";
    uint32_t width = 1600;
    uint32_t height = 900;
    bool fullscreen = true;
    bool vsync = true;
};

using logging::LogItem;

class Application : public sponge::core::Application {
   public:
    explicit Application(ApplicationSpecification specification);

    ~Application() override;

    bool start() override;

    bool iterateLoop() override;

    void shutdown() override;

    bool onUserCreate() override;

    bool onUserUpdate(double elapsedTime) override;

    bool onUserDestroy() override;

    void onEvent(event::Event& event) override;

    void onImGuiRender() const;

    void adjustAspectRatio(uint32_t eventW, uint32_t eventH);

    void pushOverlay(const std::shared_ptr<layer::Layer>& layer) const;

    void pushLayer(const std::shared_ptr<layer::Layer>& layer) const;

    void popLayer(const std::shared_ptr<layer::Layer>& layer) const;

    void popOverlay(const std::shared_ptr<layer::Layer>& layer) const;

    void toggleFullscreen();

    bool isFullscreen() const {
        return fullscreen;
    }

    layer::LayerStack* getLayerStack() const {
        return layerStack;
    }

    uint32_t getHeight() const {
        return h;
    }

    uint32_t getWidth() const {
        return w;
    }

    uint32_t getOffsetX() const {
        return offsetx;
    }

    uint32_t getOffsetY() const {
        return offsety;
    }

    bool hasVerticalSync() const {
        return vsync;
    }

    void setVerticalSync(const bool val) {
        vsync = val;
        glfwSwapInterval(vsync ? 1 : 0);
        SPONGE_CORE_DEBUG("Set vsync to {}", vsync);
    }

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

    static Application& get() {
        return *instance;
    }

    void run() override;

    std::shared_ptr<Window> window;

    bool isEventHandledByImGui() const {
        return imguiManager->isEventHandled();
    }

   private:
    std::string appName = "undefined";
    std::unique_ptr<opengl::renderer::Context> graphics;
    std::unique_ptr<opengl::renderer::RendererAPI> renderer;

    std::unique_ptr<std::vector<LogItem>> messages;

    bool fullscreen;
    bool vsync;
    int32_t prevH = 0;
    int32_t prevW = 0;
    int32_t prevX = 0;
    int32_t prevY = 0;
    uint32_t h = 0;
    uint32_t offsetx = 0;
    uint32_t offsety = 0;
    uint32_t w = 0;

    layer::LayerStack* layerStack;

    ApplicationSpecification appSpec;

    static Application* instance;

    std::shared_ptr<imgui::GLFWManager> glfwManager =
        std::make_shared<imgui::GLFWManager>();
    std::shared_ptr<imgui::NoopManager> noopManager =
        std::make_shared<imgui::NoopManager>();

#if defined(ENABLE_IMGUI)
    std::shared_ptr<imgui::GLFWManager> imguiManager = glfwManager;
#else
    std::shared_ptr<imgui::NoopManager> imguiManager = noopManager;
#endif
};
}  // namespace sponge::platform::glfw::core
