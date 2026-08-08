#pragma once
#include "core/window.hpp"
#include "event/event.hpp"
#include "logging/log.hpp"

#include <cstdint>
#include <functional>
#include <string_view>
#include <vector>

struct GLFWwindow;

namespace sponge::platform::glfw::core {

using EventCallbackFn = std::function<void(event::Event&)>;

struct WindowData {
    std::string_view title;
    uint32_t         width  = 0;
    uint32_t         height = 0;
    EventCallbackFn  eventCallback;
};

class Window final {
public:
    explicit Window(const sponge::core::WindowProps& props);
    ~Window() noexcept;

    std::string_view getTitle() const {
        return data.title;
    }

    void setTitle(const std::string_view& title) {
        data.title = title;
    }

    uint32_t getWidth() const {
        return data.width;
    }

    uint32_t getHeight() const {
        return data.height;
    }

    void* getNativeWindow() const {
        return window;
    }

    void setEventCallback(const EventCallbackFn& callback) {
        data.eventCallback = callback;
    }

    static std::vector<sponge::core::Resolution> getAvailableResolutions();

private:
    void init(const sponge::core::WindowProps& props);
    void shutdown() const;

    WindowData  data;
    GLFWwindow* window = nullptr;
};

}  // namespace sponge::platform::glfw::core
