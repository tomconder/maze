#include "layer/buttonmenulayer.hpp"

#include "input/gameaction.hpp"
#include "input/inputcontext.hpp"
#include "input/mousecode.hpp"
#include "maze.hpp"
#include "platform/glfw/core/application.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"
#include "ui/menufontsize.hpp"
#include "ui/menuselection.hpp"

#include <glm/glm.hpp>

#include <string>
#include <utility>

namespace {
constexpr std::string_view fontName  = "inter";
constexpr std::string_view fontPath  = "/fonts/inter.ktx2";
constexpr glm::vec3        textColor = { 1.F, 1.F, 1.F };
}  // namespace

namespace game::layer {
using sponge::event::Event;
using sponge::event::EventDispatcher;
using sponge::event::MouseButtonPressedEvent;
using sponge::event::MouseMovedEvent;
using sponge::event::WindowResizeEvent;
using sponge::platform::glfw::core::Application;
using sponge::platform::opengl::renderer::AssetManager;
using sponge::platform::opengl::scene::FontCreateInfo;
using sponge::platform::opengl::scene::Quad;

ButtonMenuLayer::ButtonMenuLayer(ButtonMenuStyle style) :
    Layer(style.name), menuStyle(std::move(style)) {}

void ButtonMenuLayer::onAttach() {
    font   = AssetManager::createFont(FontCreateInfo{
        .name = std::string(fontName),
        .path = std::string(fontPath),
    });
    camera = scene::menuOrthoCamera();
    if (menuStyle.backgroundColor) {
        quad = std::make_unique<Quad>();
    }

    const auto fontSize = ui::menuFontSizeForWidth(camera->getWidth());
    for (const auto label : menuStyle.labels) {
        buttons.push_back(ui::makeMenuButton(label, fontSize, font,
                                             menuStyle.buttonColor, textColor));
    }

    for (const auto& shader : { font->getShader(), Quad::getShader() }) {
        shader->setMat4("projection", camera->getProjection());
    }

    layout = std::make_unique<ui::MenuLayout>(buttons.size(), menuStyle.layout);
    recalculateLayout();
}

void ButtonMenuLayer::onDetach() {
    layout.reset();
}

void ButtonMenuLayer::onEvent(Event& event) {
    EventDispatcher dispatcher(event);

    dispatcher.dispatch<MouseButtonPressedEvent>(
        [this](const MouseButtonPressedEvent& mbEvent) {
            return isActive() ? onMouseButtonPressed(mbEvent) : false;
        });
    dispatcher.dispatch<MouseMovedEvent>(
        [this](const MouseMovedEvent& mmEvent) {
            return isActive() ? onMouseMoved(mmEvent) : false;
        });
    dispatcher.dispatch<WindowResizeEvent>(
        [this](const WindowResizeEvent& wrEvent) {
            return onWindowResize(wrEvent);
        });
}

bool ButtonMenuLayer::takesInput() const {
    // the options screen sits on top of the menu, so the menu is not taking
    // input while it is open
    return !Maze::get().isOptionsOpen();
}

void ButtonMenuLayer::handleInput() {
    auto& mgr = Application::get().getInputManager();
    mgr.setActiveContext(sponge::input::InputContext::Menu);

    using sponge::input::GameAction;
    const bool  takes = takesInput();
    const auto& input = mgr.getSnapshot();
    confirmGate.update(takes, input.isHeld(GameAction::MenuConfirm));

    if (confirmGate.wasActive && takes) {
        if (ui::stepSelection(input, selected, buttons.size())) {
            ui::playHoverClick();
        }
        if (input.isActive(GameAction::MenuBack) && onBack()) {
            mgr.consumeActive(GameAction::MenuBack);
        }
        if (confirmGate.ready() && input.isActive(GameAction::MenuConfirm)) {
            mgr.consumeActive(GameAction::MenuConfirm);
            activate(selected);
        }
    }
    confirmGate.end(takes);
}

bool ButtonMenuLayer::onUpdate(const double elapsedTime) {
    handleInput();

    if (Maze::get().isOptionsOpen()) {
        return true;
    }

    for (const auto& shader : { font->getShader(), Quad::getShader() }) {
        shader->setMat4("projection", camera->getProjection());
    }

    const auto width  = static_cast<float>(camera->getWidth());
    const auto height = static_cast<float>(camera->getHeight());

    if (quad) {
        quad->render({ 0.F, 0.F }, { width, height },
                     *menuStyle.backgroundColor);
    }

    for (size_t i = 0; i < buttons.size(); i++) {
        const auto [x, y, w, h] = layout->rowRect(i);
        buttons[i]->setPosition({ x, y }, { x + w, y + h });
        ui::updateMenuButtonVisuals(buttons[i].get(), selected == i,
                                    menuStyle.selectedColor);
        buttons[i]->onUpdate(elapsedTime);
    }

    ui::renderKeyHints(menuStyle.keyHints, font, camera->getProjection(), width,
                       height);

    if (!isActive()) {
        confirmGate.reset();
        selected = 0;
    }

    return true;
}

void ButtonMenuLayer::activate(const size_t item) {
    selected = item;
    onSelect(item);
}

void ButtonMenuLayer::clearHoveredItems() const {
    for (const auto& button : buttons) {
        button->setHover(false);
    }
}

void ButtonMenuLayer::recalculateLayout() const {
    layout->calculate(static_cast<float>(camera->getWidth()),
                      static_cast<float>(camera->getHeight()));
}

bool ButtonMenuLayer::onMouseButtonPressed(
    const MouseButtonPressedEvent& event) {
    if (!takesInput() ||
        event.getMouseButton() != sponge::input::MouseButton::Button0) {
        return false;
    }

    const auto [x, y] = Application::get().getMousePosition();
    for (size_t i = 0; i < buttons.size(); i++) {
        if (buttons[i]->isInside({ x, y })) {
            activate(i);
            return true;
        }
    }
    return menuStyle.modal;
}

bool ButtonMenuLayer::onMouseMoved(const MouseMovedEvent& event) {
    const auto pos = glm::vec2{ event.getX(), event.getY() };
    for (const auto& button : buttons) {
        ui::updateButtonHover(button.get(), pos);
    }
    return menuStyle.modal;
}

bool ButtonMenuLayer::onWindowResize(const WindowResizeEvent& event) {
    camera->setWidthAndHeight(event.getWidth(), event.getHeight());
    recalculateLayout();

    const auto fontSize = ui::menuFontSizeForWidth(event.getWidth());
    for (const auto& button : buttons) {
        button->setFontSize(fontSize);
    }
    return false;
}
}  // namespace game::layer
