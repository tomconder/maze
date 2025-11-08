#pragma once
#include "core/window.hpp"
#include "event/event.hpp"
#include "logging/log.hpp"

#include <GLFW/glfw3.h>

#include <functional>
#include <string>

namespace sponge::platform::glfw::core {

using EventCallbackFn = std::function<void(event::Event&)>;

struct WindowData {
    std::string     title;
    uint32_t        width;
    uint32_t        height;
    EventCallbackFn eventCallback;
};

class Window final : public sponge::core::Window {
public:
    explicit Window(const sponge::core::WindowProps& props);
    ~Window() noexcept override;

    uint32_t getWidth() const override {
        return data.width;
    }

    uint32_t getHeight() const override {
        return data.height;
    }

    void* getNativeWindow() const override {
        return window;
    }

    void setEventCallback(const EventCallbackFn& callback) {
        data.eventCallback = callback;
    }

private:
    void init(const sponge::core::WindowProps& props);
    void shutdown() const;

    WindowData  data;
    GLFWwindow* window;
};

}  // namespace sponge::platform::glfw::core
