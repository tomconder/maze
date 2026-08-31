#include "layer/exitlayer.hpp"

#include "core/base.hpp"
#include "input/gameaction.hpp"
#include "input/inputcontext.hpp"
#include "input/mousecode.hpp"
#include "maze.hpp"
#include "platform/glfw/core/application.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"
#include "platform/opengl/scene/bitmapfont.hpp"
#include "platform/opengl/scene/quad.hpp"
#include "resourcemanager.hpp"
#include "scene/orthocamera.hpp"
#include "ui/button.hpp"
#include "ui/keyhints.hpp"
#include "ui/menufontsize.hpp"
#include "ui/menulayout.hpp"
#include "ui/menuselection.hpp"

#include <glm/glm.hpp>
#include <yoga/Yoga.h>

#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace {
using game::layer::ExitMenuItem;

constexpr size_t menuItemCount = static_cast<size_t>(ExitMenuItem::Count);

// Rows are laid out top-to-bottom in ExitMenuItem order, so the enum doubles
// as the button and row index.
constexpr std::array<std::string_view, menuItemCount> menuLabels = {
    "Continue",
    "Options",
    "Return to Menu",
    "Exit the Game",
};

constexpr std::string_view cameraName = "exit";
constexpr std::string_view fontName   = "inter";
constexpr std::string_view fontPath   = "/fonts/inter.ttf";

constexpr glm::vec4 buttonColor    = { 0.05F, 0.05F, 0.05F, 1.F };
constexpr glm::vec3 textColor      = { 1.F, 1.F, 1.F };
constexpr glm::vec4 textHoverColor = { 0.84F, 0.04F, 0.04F, 0.07F };

std::shared_ptr<sponge::platform::opengl::scene::BitmapFont> menuFont;

std::array<std::unique_ptr<game::ui::Button>, menuItemCount> menuButtons;
std::array<YGNodeRef, menuItemCount>                         menuNodes{};

YGNodeRef menuBackgroundNode = nullptr;
YGNodeRef menuNode           = nullptr;
YGNodeRef rootNode           = nullptr;

std::shared_ptr<game::scene::OrthoCamera> orthoCamera;

constexpr std::array<game::ui::KeyHint, 3> exitKeyHints = {
    game::ui::KeyHint{ "keyboard_arrows_vertical", "xbox_dpad_vertical",
                       "Navigate" },
    game::ui::KeyHint{ "keyboard_enter", "xbox_button_a", "Select" },
    game::ui::KeyHint{ "keyboard_escape", "xbox_button_b", "Back" },
};

bool isRunning = true;
}  // namespace

namespace game::layer {
using sponge::event::Event;
using sponge::event::EventDispatcher;
using sponge::event::MouseButtonPressedEvent;
using sponge::event::MouseMovedEvent;
using sponge::event::WindowResizeEvent;
using sponge::input::GameAction;
using sponge::platform::glfw::core::Application;
using sponge::platform::opengl::renderer::AssetManager;
using sponge::platform::opengl::scene::FontCreateInfo;
using sponge::platform::opengl::scene::Quad;

ExitLayer::ExitLayer() : Layer("exit") {}

void ExitLayer::onAttach() {
    const auto fontCreateInfo = FontCreateInfo{
        .name = std::string(fontName),
        .path = std::string(fontPath),
    };
    menuFont = AssetManager::createFont(fontCreateInfo);

    const auto orthoCameraCreateInfo =
        scene::OrthoCameraCreateInfo{ .name = std::string(cameraName) };
    orthoCamera = ResourceManager::createOrthoCamera(orthoCameraCreateInfo);

    for (size_t i = 0; i < menuButtons.size(); i++) {
        menuButtons[i] = ui::makeMenuButton(
            menuLabels[i], ui::menuFontSizeForWidth(orthoCamera->getWidth()),
            menuFont, buttonColor, textColor);
    }

    for (const auto& shader : { Quad::getShader(), menuFont->getShader() }) {
        shader->bind();
        shader->setMat4("projection", orthoCamera->getProjection());
        shader->unbind();
    }

    const auto skeleton = ui::buildMenuSkeleton(45.F);
    rootNode            = skeleton.root;
    menuNode            = skeleton.menu;
    menuBackgroundNode  = skeleton.menuBackground;

    for (size_t i = 0; i < menuNodes.size(); i++) {
        menuNodes[i] = ui::makeMenuRow(menuBackgroundNode, static_cast<int>(i));
    }

    const auto width  = static_cast<float>(orthoCamera->getWidth());
    const auto height = static_cast<float>(orthoCamera->getHeight());
    recalculateLayout(width, height);
}

void ExitLayer::onDetach() {
    YGNodeFreeRecursive(rootNode);
}

void ExitLayer::onEvent(Event& event) {
    EventDispatcher dispatcher(event);

    dispatcher.dispatch<MouseButtonPressedEvent>(
        [this](const MouseButtonPressedEvent& mbEvent) {
            return isActive() ? this->onMouseButtonPressed(mbEvent) : false;
        });
    dispatcher.dispatch<MouseMovedEvent>(
        [this](const MouseMovedEvent& mmEvent) {
            return isActive() ? onMouseMoved(mmEvent) : false;
        });
    dispatcher.dispatch<WindowResizeEvent>(
        [](const WindowResizeEvent& wrEvent) {
            return onWindowResize(wrEvent);
        });
}

bool ExitLayer::onUpdate(const double elapsedTime) {
    {
        auto& mgr = Application::get().getInputManager();
        mgr.setActiveContext(sponge::input::InputContext::Menu);

        {
            using sponge::input::GameAction;
            const auto& input = mgr.getSnapshot();
            if (!wasActiveLastFrame) {
                waitForConfirmRelease = input.isHeld(GameAction::MenuConfirm);
            } else if (waitForConfirmRelease &&
                       !input.isHeld(GameAction::MenuConfirm)) {
                waitForConfirmRelease = false;
            }
        }

        if (wasActiveLastFrame && !Maze::get().isOptionsOpen()) {
            const auto& input = mgr.getSnapshot();

            if (ui::stepSelection(input, selectedItem)) {
                ui::playHoverClick();
            }

            if (input.isActive(GameAction::MenuBack)) {
                mgr.consumeActive(GameAction::MenuBack);
                selectedItem = ExitMenuItem::Continue;
                activateSelected();
            }
            if (!waitForConfirmRelease &&
                input.isActive(GameAction::MenuConfirm)) {
                mgr.consumeActive(GameAction::MenuConfirm);
                activateSelected();
            }
        }
        wasActiveLastFrame = true;
    }

    if (Maze::get().isOptionsOpen()) {
        return isRunning;
    }

    for (const auto& shader : { menuFont->getShader(), Quad::getShader() }) {
        shader->bind();
        shader->setMat4("projection", orthoCamera->getProjection());
        shader->unbind();
    }

    auto [rootNodeX, rootNodeY, rootNodeW, rootNodeH] =
        ui::getNodeLayout(rootNode, 0.F, 0.F);
    auto [menuNodeX, menuNodeY, menuNodeW, menuNodeH] =
        ui::getNodeLayout(menuNode, rootNodeX, rootNodeY);
    auto [menuBackgroundNodeX, menuBackgroundNodeY, menuBackgroundNodeW,
          menuBackgroundNodeH] =
        ui::getNodeLayout(menuBackgroundNode, menuNodeX, menuNodeY);

    ui::renderKeyHints(exitKeyHints, menuFont, orthoCamera->getProjection(),
                       static_cast<float>(orthoCamera->getWidth()),
                       static_cast<float>(orthoCamera->getHeight()));

    for (size_t i = 0; i < menuButtons.size(); i++) {
        const auto [x, y, w, h] = ui::getNodeLayout(
            menuNodes[i], menuBackgroundNodeX, menuBackgroundNodeY);

        menuButtons[i]->setPosition({ x, y }, { x + w, y + h });
        ui::updateMenuButtonVisuals(menuButtons[i].get(),
                                    static_cast<size_t>(selectedItem) == i,
                                    textHoverColor);
        UNUSED(menuButtons[i]->onUpdate(elapsedTime));
    }

    if (!isActive()) {
        wasActiveLastFrame    = false;
        waitForConfirmRelease = false;
        selectedItem          = ExitMenuItem::Continue;
    }

    return isRunning;
}

bool ExitLayer::onWindowResize(const WindowResizeEvent& event) {
    orthoCamera->setWidthAndHeight(event.getWidth(), event.getHeight());

    const auto [width, height] =
        std::pair{ static_cast<float>(event.getWidth()),
                   static_cast<float>(event.getHeight()) };
    recalculateLayout(width, height);

    const auto newFontSize = ui::menuFontSizeForWidth(event.getWidth());
    for (const auto& button : menuButtons) {
        button->setFontSize(newFontSize);
    }

    return false;
}

void ExitLayer::recalculateLayout(float width, float height) {
    const auto panelWidth = width * 0.54F;
    // leave room for the key hint bar along the bottom
    const auto usableHeight = ui::heightWithoutKeyHints(width, height);
    YGNodeStyleSetWidth(rootNode, panelWidth);
    YGNodeStyleSetHeight(rootNode, usableHeight);
    YGNodeCalculateLayout(rootNode, panelWidth, usableHeight, YGDirectionLTR);
}

bool ExitLayer::onMouseButtonPressed(const MouseButtonPressedEvent& event) {
    if (event.getMouseButton() != sponge::input::MouseButton::Button0) {
        return false;
    }

    auto [x, y] = Application::get().getMousePosition();

    for (size_t i = 0; i < menuButtons.size(); i++) {
        if (!menuButtons[i]->isInside({ x, y })) {
            continue;
        }
        selectedItem = static_cast<ExitMenuItem>(i);
        activateSelected();
        break;
    }

    return true;
}

void ExitLayer::activateSelected() {
    switch (selectedItem) {
        case ExitMenuItem::Continue:
            clearHoveredItems();
            resumeGame();
            break;
        case ExitMenuItem::Options:
            clearHoveredItems();
            Maze::get().getOptionLayer()->setActive(true);
            break;
        case ExitMenuItem::ReturnToMenu:
            clearHoveredItems();
            Maze::get().getIntroLayer()->setActive(true);
            Maze::get().getMazeLayer()->setActive(false);
            setActive(false);
            break;
        case ExitMenuItem::Exit:
            isRunning = false;
            break;
        default:
            break;
    }
}

bool ExitLayer::onMouseMoved(const MouseMovedEvent& event) {
    const auto pos = glm::vec2{ event.getX(), event.getY() };

    for (const auto& button : menuButtons) {
        ui::updateButtonHover(button.get(), pos);
    }

    return true;
}

void ExitLayer::resumeGame() {
    setActive(false);
    if (Maze::get().getMazeLayer()->isImguiActive()) {
        Maze::get().getImGuiLayer()->setActive(true);
    }
}

void ExitLayer::clearHoveredItems() {
    for (const auto& button : menuButtons) {
        button->setHover(false);
    }
}
}  // namespace game::layer
