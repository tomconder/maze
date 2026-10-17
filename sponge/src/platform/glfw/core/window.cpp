#include "platform/glfw/core/window.hpp"

#include "event/applicationevent.hpp"
#include "event/mouseevent.hpp"
#include "logging/log.hpp"
#include "platform/glfw/core/application.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <span>
#include <string>
#include <vector>

namespace sponge::platform::glfw::core {
Window::Window(const sponge::core::WindowProps& props) {
    init(props);
}

Window::~Window() noexcept {
    shutdown();
}

namespace {

WindowData* dataOf(GLFWwindow* win) {
    return static_cast<WindowData*>(glfwGetWindowUserPointer(win));
}

GLFWwindow* createGlfwWindow(const sponge::core::WindowProps& props,
                             GLFWmonitor*                     primaryMonitor,
                             const GLFWvidmode*               mode) {
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    if (mode != nullptr) {
        glfwWindowHint(GLFW_RED_BITS, mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
    }

    auto width  = static_cast<int32_t>(props.width);
    auto height = static_cast<int32_t>(props.height);

    if (width == 0 && height == 0) {
        // Matches ApplicationSpecification's default when no monitor mode is
        // available to size the window against.
        width  = mode != nullptr ? mode->width : 1600;
        height = mode != nullptr ? mode->height : 900;
    }

    using sponge::core::DisplayMode;
    if (props.displayMode == DisplayMode::FullScreen) {
        SPONGE_CORE_INFO("Creating fullscreen window");
        return glfwCreateWindow(width, height, props.title.data(),
                                primaryMonitor, nullptr);
    }
    if (props.displayMode == DisplayMode::Borderless && mode != nullptr) {
        SPONGE_CORE_INFO("Creating borderless window");
        return glfwCreateWindow(mode->width, mode->height, props.title.data(),
                                nullptr, nullptr);
    }
    SPONGE_CORE_INFO("Creating window {}x{}", props.width, props.height);
    return glfwCreateWindow(width, height, props.title.data(), nullptr,
                            nullptr);
}

void positionWindow(GLFWwindow* window, const sponge::core::WindowProps& props,
                    GLFWmonitor* primaryMonitor, const GLFWvidmode* mode) {
    using sponge::core::DisplayMode;
    if (mode == nullptr) {
        return;
    }

    if (props.displayMode == DisplayMode::Windowed) {
        int w = 0;
        int h = 0;
        glfwGetWindowSize(window, &w, &h);
        glfwSetWindowPos(window, (mode->width - w) / 2, (mode->height - h) / 2);
    } else if (props.displayMode == DisplayMode::Borderless) {
        int monitorX = 0;
        int monitorY = 0;
        glfwGetMonitorPos(primaryMonitor, &monitorX, &monitorY);
        glfwSetWindowPos(window, monitorX, monitorY);
    }
}

void installCallbacks(GLFWwindow* window) {
    glfwSetWindowSizeCallback(
        window, [](GLFWwindow* win, const int newWidth, const int newHeight) {
            if (newWidth == 0 && newHeight == 0) {
                return;
            }

            auto* data = dataOf(win);

            data->width  = static_cast<uint32_t>(newWidth);
            data->height = static_cast<uint32_t>(newHeight);

            // Defer viewport GL call to render thread; callback may fire on any
            // thread.
            Application::get().setPendingViewport(newWidth, newHeight);

            event::WindowResizeEvent event(static_cast<uint32_t>(newWidth),
                                           static_cast<uint32_t>(newHeight));
            data->eventCallback(event);
        });

    glfwSetWindowCloseCallback(window, [](GLFWwindow* win) {
        event::WindowCloseEvent event;
        dataOf(win)->eventCallback(event);
    });

    glfwSetMouseButtonCallback(window, [](GLFWwindow* win, const int button,
                                          const int action, const int) {
        if (Application::get().isEventHandledByImGui()) {
            return;
        }

        const auto* data       = dataOf(win);
        const auto  buttonCode = static_cast<input::MouseButton>(button);

        switch (action) {
            case GLFW_PRESS: {
                event::MouseButtonPressedEvent event(buttonCode);
                data->eventCallback(event);
                break;
            }
            case GLFW_RELEASE: {
                event::MouseButtonReleasedEvent event(buttonCode);
                data->eventCallback(event);
                break;
            }
            default:
                break;
        }
    });

    glfwSetCursorPosCallback(
        window, [](GLFWwindow* win, const double x, const double y) {
            if (Application::get().isEventHandledByImGui()) {
                return;
            }

            event::MouseMovedEvent event(0.F, 0.F, static_cast<float>(x),
                                         static_cast<float>(y));
            dataOf(win)->eventCallback(event);
        });

    glfwSetScrollCallback(window, [](GLFWwindow* win, const double xoffset,
                                     const double yoffset) {
        if (Application::get().isEventHandledByImGui()) {
            return;
        }
        event::MouseScrolledEvent event(static_cast<float>(xoffset),
                                        static_cast<float>(yoffset));
        dataOf(win)->eventCallback(event);
    });

    glfwSetWindowFocusCallback(window, [](GLFWwindow* win, const int focused) {
        event::WindowFocusEvent event(focused == GLFW_TRUE);
        dataOf(win)->eventCallback(event);
    });
}

}  // namespace

void Window::init(const sponge::core::WindowProps& props) {
    data.title  = props.title;
    data.width  = props.width;
    data.height = props.height;

    if (props.title.empty()) {
        SPONGE_CORE_CRITICAL("Title cannot be empty");
    }

    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);

    GLFWmonitor*       primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode =
        primaryMonitor != nullptr ? glfwGetVideoMode(primaryMonitor) : nullptr;
    if (mode == nullptr) {
        SPONGE_CORE_ERROR("No primary monitor/video mode detected");
    }

    window = createGlfwWindow(props, primaryMonitor, mode);

    if (window == nullptr) {
        const char* descCStr = nullptr;
        glfwGetError(&descCStr);
        const std::string description = descCStr ? descCStr : "Unknown error";
        SPONGE_CORE_CRITICAL("Could not create window: {}", description);
        return;
    }

    int w = 0;
    int h = 0;
    glfwGetWindowSize(window, &w, &h);

    data.width  = static_cast<uint32_t>(w);
    data.height = static_cast<uint32_t>(h);

    positionWindow(window, props, primaryMonitor, mode);

    using sponge::core::DisplayMode;
    glfwSetWindowAttrib(window, GLFW_DECORATED,
                        props.displayMode == DisplayMode::Windowed &&
                                props.decorated ?
                            GLFW_TRUE :
                            GLFW_FALSE);

    glfwSetWindowUserPointer(window, &data);
    installCallbacks(window);
}

void Window::shutdown() const {
    glfwDestroyWindow(window);
    glfwTerminate();
}

std::vector<sponge::core::Resolution> Window::getAvailableResolutions() {
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    if (primaryMonitor == nullptr) {
        return {};
    }

    const GLFWvidmode* currentMode = glfwGetVideoMode(primaryMonitor);
    if (currentMode == nullptr) {
        return {};
    }

    const int targetRefreshRate = currentMode->refreshRate;

    int                count = 0;
    const GLFWvidmode* modes = glfwGetVideoModes(primaryMonitor, &count);
    if (modes == nullptr || count == 0) {
        return {};
    }

    auto filtered =
        std::span(modes, static_cast<size_t>(count)) |
        std::views::filter([targetRefreshRate](const GLFWvidmode& m) {
            return m.refreshRate == targetRefreshRate && m.width >= 1024;
        }) |
        std::views::transform(
            [](const GLFWvidmode& m) -> sponge::core::Resolution {
                return {
                    .width  = static_cast<uint32_t>(m.width),
                    .height = static_cast<uint32_t>(m.height),
                };
            });

    auto resolutions = std::vector(filtered.begin(), filtered.end());

    std::ranges::sort(resolutions, [](const auto& a, const auto& b) {
        return a.width != b.width ? a.width < b.width : a.height < b.height;
    });

    return resolutions;
}
}  // namespace sponge::platform::glfw::core
