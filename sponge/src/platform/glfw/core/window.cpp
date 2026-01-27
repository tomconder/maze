#include "platform/glfw/core/window.hpp"

#include "event/applicationevent.hpp"
#include "event/keyevent.hpp"
#include "event/mouseevent.hpp"
#include "logging/log.hpp"
#include "platform/glfw/core/application.hpp"
#include "platform/glfw/core/input.hpp"

#include <array>
#include <ranges>

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

std::tuple<uint32_t, uint32_t, uint32_t, uint32_t>
    Window::adjustAspectRatio(uint32_t width, uint32_t height) {
    float proposedRatio =
        static_cast<float>(width) / static_cast<float>(height);
    auto exceedsRatio = [&proposedRatio](const glm::vec3 i) {
        return proposedRatio >= i.z;
    };

    glm::vec3 ratio;
    if (const auto it = std::ranges::find_if(ratios, exceedsRatio);
        it != std::end(ratios)) {
        ratio = *it;
    } else {
        ratio = *(ratios.end() - 1);
    }

    const float aspectRatioWidth  = ratio.x;
    const float aspectRatioHeight = ratio.y;

    const float aspectRatio = aspectRatioWidth / aspectRatioHeight;

    const auto fw = static_cast<float>(width);
    const auto fh = static_cast<float>(height);

    uint32_t w = 0;
    uint32_t h = 0;

    if (const float newAspectRatio = fw / fh; newAspectRatio > aspectRatio) {
        w = static_cast<int>(aspectRatioWidth * fh / aspectRatioHeight);
        h = static_cast<int>(fh);
    } else {
        w = static_cast<int>(fw);
        h = static_cast<int>(aspectRatioHeight * fw / aspectRatioWidth);
    }

    data.width   = w;
    data.height  = h;
    data.offsetx = static_cast<uint32_t>((width - w) / 2.F);
    data.offsety = static_cast<uint32_t>((height - h) / 2.F);

    SPONGE_CORE_DEBUG(fmt::format("Resizing viewport to {}x{}", w, h));

    return { data.width, data.height, data.offsetx, data.offsety };
}

void Window::init(const sponge::core::WindowProps& props) {
    data.title  = props.title;
    data.width  = props.width;
    data.height = props.height;

    if (props.title.empty()) {
        SPONGE_CORE_CRITICAL("Title cannot be empty");
    }

    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);

    if (props.fullscreen) {
        SPONGE_CORE_INFO("Creating fullscreen window");

        GLFWmonitor*       primaryMonitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode           = glfwGetVideoMode(primaryMonitor);

        glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
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

        window = glfwCreateWindow(width, height, props.title.data(),
                                  primaryMonitor, nullptr);
    } else {
        SPONGE_CORE_INFO("Creating window {}x{}", props.width, props.height);

        window = glfwCreateWindow(static_cast<int>(props.width),
                                  static_cast<int>(props.height),
                                  props.title.data(), nullptr, nullptr);
    }

    if (window == nullptr) {
        const char* descCStr = nullptr;
        glfwGetError(&descCStr);
        const std::string description = descCStr ? descCStr : "Unknown error";
        SPONGE_CORE_CRITICAL("Could not create window: {}", description);
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

        event::WindowResizeEvent event(adjustedW, adjustedH);
        data->eventCallback(event);
        data->width  = adjustedW;
        data->height = adjustedH;
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
}  // namespace sponge::platform::glfw::core
