#include "window.hpp"
#include "event/applicationevent.hpp"
#include "event/keyevent.hpp"
#include "event/mouseevent.hpp"
#include "logging/log.hpp"
#include "platform/glfw/core/application.hpp"
#include "platform/glfw/core/input.hpp"

namespace sponge::platform::glfw::core {
Window::Window(const sponge::core::WindowProps& props) {
    init(props);
}

Window::~Window() noexcept {
    shutdown();
}

void Window::init(const sponge::core::WindowProps& props) {
    data.title = props.title;
    data.width = props.width;
    data.height = props.height;

    if (props.title.empty()) {
        SPONGE_CORE_CRITICAL("Title cannot be empty");
    }

    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);

    if (props.fullscreen) {
        SPONGE_CORE_INFO("Creating fullscreen window");

        GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

        glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
        glfwWindowHint(GLFW_RED_BITS, mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

        window = glfwCreateWindow(static_cast<int>(props.width),
                                  static_cast<int>(props.height),
                                  props.title.c_str(), primaryMonitor, nullptr);
    } else {
        SPONGE_CORE_INFO("Creating window {}x{}", props.width, props.height);

        window = glfwCreateWindow(static_cast<int>(props.width),
                                  static_cast<int>(props.height),
                                  props.title.c_str(), nullptr, nullptr);
    }

    if (window == nullptr) {
        const char* description = nullptr;
        glfwGetError(&description);
        SPONGE_CORE_CRITICAL("Could not create window: {}", description);
    }

    int w = 0;
    int h = 0;
    glfwGetWindowSize(window, &w, &h);

    data.width = static_cast<uint32_t>(w);
    data.height = static_cast<uint32_t>(h);

    glfwSetWindowAttrib(window, GLFW_DECORATED,
                        props.fullscreen ? GLFW_FALSE : GLFW_TRUE);

    glfwSetWindowUserPointer(window, &data);

    glfwSetWindowSizeCallback(window, [](GLFWwindow* window, const int width,
                                         const int height) {
        auto w = static_cast<uint32_t>(width);
        auto h = static_cast<uint32_t>(height);

        if (w == 0 && h == 0) {
            return;
        }

        Application::get().adjustAspectRatio(w, h);

        auto* data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));

        event::WindowResizeEvent event(w, h);
        data->eventCallback(event);
        data->width = w;
        data->height = h;
    });

    glfwSetWindowCloseCallback(window, [](GLFWwindow* window) {
        const auto* data =
            static_cast<WindowData*>(glfwGetWindowUserPointer(window));

        event::WindowCloseEvent event;
        data->eventCallback(event);
    });

    glfwSetKeyCallback(
        window, [](GLFWwindow* window, const int key, const int scancode,
                   const int action, const int mods) {
            UNUSED(scancode);
            UNUSED(mods);

            if (Application::get().isEventHandledByImGui()) {
                return;
            }

            const auto* data =
                static_cast<WindowData*>(glfwGetWindowUserPointer(window));

            const auto keycode = static_cast<input::KeyCode>(key);

            switch (action) {
                case GLFW_PRESS: {
                    Input::updateKeyState(keycode, KeyState::Pressed);
                    event::KeyPressedEvent event(keycode);
                    data->eventCallback(event);
                    break;
                }
                case GLFW_RELEASE: {
                    Input::updateKeyState(keycode, KeyState::Released);
                    event::KeyReleasedEvent event(keycode);
                    data->eventCallback(event);
                    break;
                }
                case GLFW_REPEAT: {
                    Input::updateKeyState(keycode, KeyState::Held);
                    event::KeyPressedEvent event(keycode, true);
                    data->eventCallback(event);
                    break;
                }
                default:
                    break;
            }
        });

    glfwSetCharCallback(window, [](GLFWwindow* window, uint32_t codepoint) {
        const auto* data =
            static_cast<WindowData*>(glfwGetWindowUserPointer(window));

        if (Application::get().isEventHandledByImGui()) {
            return;
        }

        event::KeyTypedEvent event(static_cast<input::KeyCode>(codepoint));
        data->eventCallback(event);
    });

    glfwSetMouseButtonCallback(window, [](GLFWwindow* window, const int button,
                                          const int action, const int mods) {
        UNUSED(mods);

        if (Application::get().isEventHandledByImGui()) {
            return;
        }

        const auto* data =
            static_cast<WindowData*>(glfwGetWindowUserPointer(window));

        const auto buttonCode = static_cast<input::MouseButton>(button);
        if (button != GLFW_MOUSE_BUTTON_LEFT) {
            return;
        }

        switch (action) {
            case GLFW_PRESS: {
                Input::updateButtonState(
                    static_cast<input::MouseButton>(button), KeyState::Pressed);
                event::MouseButtonPressedEvent event(buttonCode);
                data->eventCallback(event);
                break;
            }
            case GLFW_RELEASE: {
                Input::updateButtonState(
                    static_cast<input::MouseButton>(button),
                    KeyState::Released);
                event::MouseButtonReleasedEvent event(buttonCode);
                data->eventCallback(event);
                break;
            }
            default:
                break;
        }
    });

    glfwSetScrollCallback(window, [](GLFWwindow* window, const double xOffset,
                                     const double yOffset) {
        if (Application::get().isEventHandledByImGui()) {
            return;
        }

        const auto* data =
            static_cast<WindowData*>(glfwGetWindowUserPointer(window));

        event::MouseScrolledEvent event(static_cast<float>(xOffset),
                                        static_cast<float>(yOffset));
        data->eventCallback(event);
    });

    glfwSetCursorPosCallback(
        window, [](GLFWwindow* window, const double x, const double y) {
            if (Application::get().isEventHandledByImGui()) {
                return;
            }

            auto [prevX, prevY] = Input::getPrevCursorPos();

            auto xrel = x - prevX;
            auto yrel = y - prevY;

            Input::setPrevCursorPos({ x, y });
            Input::setRelativeCursorPos({ xrel, yrel });

            const auto* data =
                static_cast<WindowData*>(glfwGetWindowUserPointer(window));
            event::MouseMovedEvent event(
                static_cast<float>(xrel), static_cast<float>(yrel),
                static_cast<float>(x), static_cast<float>(y));

            data->eventCallback(event);
        });

    glfwSetCursorEnterCallback(window,
                               [](GLFWwindow* window, const int entered) {
                                   double x = 0;
                                   double y = 0;
                                   glfwGetCursorPos(window, &x, &y);
                                   Input::setPrevCursorPos({ x, y });
                                   Input::setCursorEnteredWindow(entered);
                               });
}

void Window::shutdown() const {
    glfwDestroyWindow(window);
    glfwTerminate();
}
} // namespace sponge::platform::glfw::core
