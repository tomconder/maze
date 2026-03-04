#include "platform/glfw/core/window.hpp"

#include "event/applicationevent.hpp"
#include "event/keyevent.hpp"
#include "event/mouseevent.hpp"
#include "logging/log.hpp"
#include "platform/glfw/core/application.hpp"
#include "platform/glfw/core/input.hpp"

#include <array>
#include <ranges>
#include <set>

namespace {
constexpr auto ratios = std::to_array(
    { glm::vec3{ 32.F, 9.F, 32.F / 9.F }, glm::vec3{ 21.F, 9.F, 21.F / 9.F },
      glm::vec3{ 16.F, 9.F, 16.F / 9.F }, glm::vec3{ 16.F, 10.F, 16.F / 10.F },
      glm::vec3{ 4.F, 3.F, 4.F / 3.F } });
}  // namespace

namespace sponge::platform::glfw::core {
Window::Window(const sponge::core::WindowProps& props) {
    window = nullptr;
    init(props);
}

Window::~Window() noexcept {
    shutdown();
}

void Window::init(const sponge::core::WindowProps& props) {
    data.title  = props.title;
    data.width  = props.width;
    data.height = props.height;

    if (props.title.empty()) {
        SPONGE_CORE_CRITICAL("Title cannot be empty");
    }

    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);

    GLFWmonitor*       primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode           = glfwGetVideoMode(primaryMonitor);

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

    auto width  = static_cast<int32_t>(props.width);
    auto height = static_cast<int32_t>(props.height);

    if (width == 0 && height == 0) {
        width  = mode->width;
        height = mode->height;
    }

    if (props.fullscreen) {
        SPONGE_CORE_INFO("Creating fullscreen window");
        window = glfwCreateWindow(width, height, props.title.data(),
                                  primaryMonitor, nullptr);
    } else {
        SPONGE_CORE_INFO("Creating window {}x{}", props.width, props.height);
        window = glfwCreateWindow(width, height, props.title.data(), nullptr,
                                  nullptr);
    }

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

    glfwSetWindowAttrib(window, GLFW_DECORATED,
                        props.fullscreen ? GLFW_FALSE : GLFW_TRUE);

    glfwSetWindowUserPointer(window, &data);

    glfwSetWindowSizeCallback(window, [](GLFWwindow* window, const int width,
                                         const int height) {
        if (width == 0 && height == 0) {
            return;
        }

        const auto [adjustedW, adjustedH] =
            Application::get().adjustAspectRatio(width, height);

        auto* data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));

        data->width  = adjustedW;
        data->height = adjustedH;

        event::WindowResizeEvent event(adjustedW, adjustedH);
        data->eventCallback(event);
    });

    glfwSetWindowCloseCallback(window, [](GLFWwindow* window) {
        const auto* data =
            static_cast<WindowData*>(glfwGetWindowUserPointer(window));

        event::WindowCloseEvent event;
        data->eventCallback(event);
    });

    glfwSetKeyCallback(window, [](GLFWwindow* window, const int key,
                                  const int scancode, const int action,
                                  const int mods) {
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

std::vector<sponge::core::Resolution> Window::getAvailableResolutions() {
    std::vector<sponge::core::Resolution> resolutions;

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    if (primaryMonitor == nullptr) {
        return resolutions;
    }

    // TODO filter by refresh rate?

    int                count = 0;
    const GLFWvidmode* modes = glfwGetVideoModes(primaryMonitor, &count);
    if (modes == nullptr || count == 0) {
        return resolutions;
    }

    // TODO do not need to dedupe if filtered by refresh rate

    // Use set to deduplicate (GLFW returns same resolution with different
    // refresh rates). Set stores in ascending order by default.
    std::set<std::pair<uint32_t, uint32_t>> uniqueResolutions;
    for (int i = 0; i < count; ++i) {
        uniqueResolutions.emplace(static_cast<uint32_t>(modes[i].width),
                                  static_cast<uint32_t>(modes[i].height));
    }

    // TODO sort by horizontal resolution then by vertical resolution

    resolutions.reserve(uniqueResolutions.size());
    for (auto it = uniqueResolutions.rbegin(); it != uniqueResolutions.rend();
         ++it) {
        resolutions.push_back({ it->first, it->second });
    }

    return resolutions;
}
}  // namespace sponge::platform::glfw::core
