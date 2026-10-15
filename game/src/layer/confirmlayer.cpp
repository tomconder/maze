#include "layer/confirmlayer.hpp"

#include "input/inputcontext.hpp"
#include "platform/glfw/core/application.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"
#include "platform/opengl/scene/bitmapfont.hpp"
#include "platform/opengl/scene/quad.hpp"
#include "scene/orthocamera.hpp"
#include "ui/menufontsize.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <utility>

namespace {
constexpr std::string_view fontName = "inter";
constexpr std::string_view fontPath = "/fonts/inter.ktx2";

std::shared_ptr<sponge::platform::opengl::scene::BitmapFont> menuFont;
std::shared_ptr<game::scene::OrthoCamera>                    orthoCamera;
}  // namespace

namespace game::layer {
using sponge::event::Event;
using sponge::event::EventDispatcher;
using sponge::event::MouseButtonPressedEvent;
using sponge::event::MouseMovedEvent;
using sponge::event::MouseScrolledEvent;
using sponge::event::WindowResizeEvent;
using sponge::platform::glfw::core::Application;
using sponge::platform::opengl::renderer::AssetManager;
using sponge::platform::opengl::scene::FontCreateInfo;
using sponge::platform::opengl::scene::Quad;

ConfirmLayer::ConfirmLayer() : Layer("confirm") {}

void ConfirmLayer::onAttach() {
    menuFont = AssetManager::createFont(FontCreateInfo{
        .name = std::string(fontName),
        .path = std::string(fontPath),
    });

    orthoCamera = scene::menuOrthoCamera();

    dialog = std::make_unique<ui::ConfirmDialog>(
        menuFont, ui::menuFontSizeForWidth(orthoCamera->getWidth()));
}

void ConfirmLayer::onEvent(Event& event) {
    EventDispatcher dispatcher(event);

    dispatcher.dispatch<MouseButtonPressedEvent>(
        [this](const MouseButtonPressedEvent& pressed) {
            return isActive() ? onMouseButtonPressed(pressed) : false;
        });
    dispatcher.dispatch<MouseMovedEvent>([this](const MouseMovedEvent& moved) {
        return isActive() ? onMouseMoved(moved) : false;
    });
    // nothing under the prompt scrolls while it is open
    dispatcher.dispatch<MouseScrolledEvent>(
        [this](const MouseScrolledEvent&) { return isActive(); });
    dispatcher.dispatch<WindowResizeEvent>(
        [](const WindowResizeEvent& resized) {
            return onWindowResize(resized);
        });
}

bool ConfirmLayer::onUpdate(const double elapsedTime) {
    auto& mgr = Application::get().getInputManager();
    mgr.setActiveContext(sponge::input::InputContext::Menu);

    // The layer under this one reads the keys first and can open the prompt,
    // so the press that opened it is not also read here.
    if (wasActiveLastFrame) {
        finish(dialog->onInput(mgr));
        if (!isActive()) {
            return true;
        }
    }
    wasActiveLastFrame = true;

    for (const auto& shader : { menuFont->getShader(), Quad::getShader() }) {
        shader->setMat4("projection", orthoCamera->getProjection());
    }
    dialog->setFontSize(ui::menuFontSizeForWidth(orthoCamera->getWidth()));
    finish(dialog->onUpdate(elapsedTime,
                            static_cast<float>(orthoCamera->getWidth()),
                            static_cast<float>(orthoCamera->getHeight())));
    return true;
}

void ConfirmLayer::show(const std::string_view                 title,
                        const std::string_view                 message,
                        std::function<void(ui::ConfirmResult)> callback,
                        const float seconds, const bool defaultAccept) {
    onResult = std::move(callback);
    dialog->open(title, message, seconds, defaultAccept);
    wasActiveLastFrame = false;
    setActive(true);
}

void ConfirmLayer::finish(const ui::ConfirmResult result) {
    if (result == ui::ConfirmResult::Pending) {
        return;
    }
    dialog->close();
    setActive(false);
    wasActiveLastFrame = false;
    if (const auto callback = std::exchange(onResult, nullptr); callback) {
        callback(result);
    }
}

bool ConfirmLayer::onMouseButtonPressed(const MouseButtonPressedEvent&) {
    const auto [x, y] = Application::get().getMousePosition();
    finish(dialog->onMousePressed({ x, y }));
    return true;
}

bool ConfirmLayer::onMouseMoved(const MouseMovedEvent& event) {
    dialog->onMouseMoved({ event.getX(), event.getY() });
    return true;
}

bool ConfirmLayer::onWindowResize(const WindowResizeEvent& event) {
    orthoCamera->setWidthAndHeight(event.getWidth(), event.getHeight());
    return false;
}
}  // namespace game::layer
