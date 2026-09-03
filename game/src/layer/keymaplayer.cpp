#include "layer/keymaplayer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>

#include <yoga/Yoga.h>

#include "core/base.hpp"
#include "event/event.hpp"
#include "input/gameaction.hpp"
#include "input/inputcontext.hpp"
#include "input/mousecode.hpp"
#include "maze.hpp"
#include "platform/glfw/core/application.hpp"
#include "platform/glfw/core/inputmanager.hpp"
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
#include "ui/tabbar.hpp"

namespace {
constexpr std::string_view cameraName = "intro";
constexpr std::string_view fontName   = "inter";
constexpr std::string_view fontPath   = "/fonts/inter.ttf";

constexpr std::string_view resetMessage   = "Reset to Defaults";
constexpr std::string_view returnMessage  = "Return";
constexpr std::string_view captureMessage = "Press a key";

constexpr glm::vec4 backgroundColor = { 0.F, 0.F, 0.F, 1.F };
constexpr glm::vec4 buttonColor     = { 0.F, 0.F, 0.F, 0.F };
constexpr glm::vec4 hoverColor      = { 0.84F, 0.84F, 0.84F, 0.14F };
constexpr glm::vec3 textColor       = { 1.F, 1.F, 1.F };
constexpr glm::vec3 captureColor    = { 1.F, 0.84F, 0.2F };
constexpr glm::vec4 textHoverColor  = { 0.84F, 0.04F, 0.04F, 0.14F };

uint32_t        fontSize            = 48;
constexpr float textMarginLeft      = 26.F;
constexpr float cornerRadius        = 12.F;
constexpr float selectedBorderWidth = 3.F;

using game::layer::KeyMapItem;

constexpr size_t rowCount = static_cast<size_t>(KeyMapItem::Count);

struct BindingRow {
    KeyMapItem                item;
    sponge::input::GameAction action;
    std::string_view          label;
};

// Menu navigation is deliberately absent: rebinding it can leave the player
// with no way back out of this screen.
constexpr std::array bindingRows = {
    BindingRow{ KeyMapItem::MoveForward, sponge::input::GameAction::MoveForward,
                "Move Forward" },
    BindingRow{ KeyMapItem::MoveBack, sponge::input::GameAction::MoveBack,
                "Move Back" },
    BindingRow{ KeyMapItem::MoveLeft, sponge::input::GameAction::MoveLeft,
                "Move Left" },
    BindingRow{ KeyMapItem::MoveRight, sponge::input::GameAction::MoveRight,
                "Move Right" },
    BindingRow{ KeyMapItem::Pause, sponge::input::GameAction::Pause, "Pause" },
    BindingRow{ KeyMapItem::ToggleFullscreen,
                sponge::input::GameAction::ToggleFullscreen, "Full Screen" },
    BindingRow{ KeyMapItem::ToggleDebugUI,
                sponge::input::GameAction::ToggleDebugUI, "Debug UI" },
};

// rowNodes are created by index, so the table must stay in enum order and
// cover every row but ResetDefaults and Return.
static_assert(bindingRows.size() + 2 == rowCount);
static_assert([] {
    for (size_t i = 0; i < bindingRows.size(); i++) {
        if (static_cast<size_t>(bindingRows[i].item) != i) {
            return false;
        }
    }
    return true;
}());

// Rows are laid out top-to-bottom in KeyMapItem order, so the enum doubles as
// the row index.
std::array<YGNodeRef, rowCount> rowNodes{};

YGNodeRef menuBackgroundNode = nullptr;
YGNodeRef menuNode           = nullptr;
YGNodeRef rootNode           = nullptr;

constexpr std::array<game::ui::KeyHint, 3> keyMapKeyHints = {
    game::ui::KeyHint{ "keyboard_arrows_vertical", "xbox_dpad_vertical",
                       "Navigate" },
    game::ui::KeyHint{ "keyboard_enter", "xbox_button_a", "Rebind" },
    game::ui::KeyHint{ "keyboard_escape", "xbox_button_b", "Back" },
};

std::unique_ptr<game::ui::Button> resetButton;
std::unique_ptr<game::ui::Button> returnButton;

std::shared_ptr<sponge::platform::opengl::scene::BitmapFont> menuFont;
std::unique_ptr<sponge::platform::opengl::scene::Quad>       quad;
std::shared_ptr<game::scene::OrthoCamera>                    orthoCamera;

sponge::platform::glfw::core::InputManager& inputManager() {
    return sponge::platform::glfw::core::Application::get().getInputManager();
}

bool contains(const float x, const float y, const float w, const float h,
              const float px, const float py) {
    return px >= x && px <= x + w && py >= y && py <= y + h;
}

game::ui::Button* buttonFor(const KeyMapItem item) {
    switch (item) {
        case KeyMapItem::ResetDefaults:
            return resetButton.get();
        case KeyMapItem::Return:
            return returnButton.get();
        default:
            return nullptr;
    }
}
}  // namespace

namespace game::layer {
using sponge::event::Event;
using sponge::event::EventDispatcher;
using sponge::event::MouseButtonPressedEvent;
using sponge::event::MouseMovedEvent;
using sponge::event::WindowResizeEvent;
using sponge::platform::opengl::renderer::AssetManager;
using sponge::platform::opengl::scene::FontCreateInfo;
using sponge::platform::opengl::scene::Quad;

KeyMapLayer::KeyMapLayer() : Layer("keymap") {}

void KeyMapLayer::onAttach() {
    const auto fontCreateInfo = FontCreateInfo{
        .name = std::string(fontName),
        .path = std::string(fontPath),
    };
    menuFont = AssetManager::createFont(fontCreateInfo);

    const auto orthoCameraCreateInfo =
        scene::OrthoCameraCreateInfo{ .name = std::string(cameraName) };
    orthoCamera = ResourceManager::createOrthoCamera(orthoCameraCreateInfo);

    quad = std::make_unique<Quad>();

    fontSize = ui::menuFontSizeForWidth(orthoCamera->getWidth());

    resetButton  = ui::makeMenuButton(resetMessage, fontSize, menuFont,
                                      buttonColor, textColor);
    returnButton = ui::makeMenuButton(returnMessage, fontSize, menuFont,
                                      buttonColor, textColor);

    for (const auto& shader : { menuFont->getShader(), Quad::getShader() }) {
        shader->bind();
        shader->setMat4("projection", orthoCamera->getProjection());
        shader->unbind();
    }

    // no spacer above: the rows start under the tab bar
    const auto skeleton = ui::buildMenuSkeleton(45.F, 0.F);
    rootNode            = skeleton.root;
    menuNode            = skeleton.menu;
    menuBackgroundNode  = skeleton.menuBackground;

    for (size_t i = 0; i < rowNodes.size(); i++) {
        rowNodes[i] = ui::makeMenuRow(menuBackgroundNode, static_cast<int>(i));
    }

    const auto width  = static_cast<float>(orthoCamera->getWidth());
    const auto height = static_cast<float>(orthoCamera->getHeight());
    recalculateLayout(width, height);
}

void KeyMapLayer::onDetach() {
    YGNodeFreeRecursive(rootNode);
}

void KeyMapLayer::onEvent(Event& event) {
    EventDispatcher dispatcher(event);

    dispatcher.dispatch<MouseButtonPressedEvent>(
        [this](const MouseButtonPressedEvent& mouseEvent) {
            return isActive() ? onMouseButtonPressed(mouseEvent) : false;
        });
    dispatcher.dispatch<MouseMovedEvent>(
        [this](const MouseMovedEvent& mouseMovedEvent) {
            return isActive() ? onMouseMoved(mouseMovedEvent) : false;
        });
    dispatcher.dispatch<WindowResizeEvent>(
        [](const WindowResizeEvent& windowResizeEvent) {
            return onWindowResize(windowResizeEvent);
        });
}

bool KeyMapLayer::onUpdate(const double elapsedTime) {
    auto& mgr = inputManager();
    mgr.setActiveContext(sponge::input::InputContext::Menu);

    // The capture frame reports no actions at all, so nothing below fires on
    // the key being bound.
    if (rebindingItem && !mgr.isRebinding()) {
        rebindingItem.reset();
    }

    // the row being bound takes the input itself, so the menu is not taking
    // input while a capture is running
    const bool active = !rebindingItem;

    {
        using sponge::input::GameAction;
        const auto& input = mgr.getSnapshot();

        if (active && !wasActiveLastFrame) {
            waitForConfirmRelease = input.isHeld(GameAction::MenuConfirm);
        } else if (waitForConfirmRelease &&
                   !input.isHeld(GameAction::MenuConfirm)) {
            waitForConfirmRelease = false;
        }

        if (wasActiveLastFrame && active) {
            if (ui::stepSelection(input, selectedItem)) {
                ui::playHoverClick();
            }
            if (input.isActive(GameAction::TabNext) ||
                input.isActive(GameAction::TabPrev)) {
                mgr.consumeActive(GameAction::TabNext);
                mgr.consumeActive(GameAction::TabPrev);
                clearHoveredItems();
                ui::showOptionTab(ui::OptionTab::Display);
            }
            if (input.isActive(GameAction::MenuBack)) {
                mgr.consumeActive(GameAction::MenuBack);
                close();
            }
            if (!waitForConfirmRelease &&
                input.isActive(GameAction::MenuConfirm)) {
                mgr.consumeActive(GameAction::MenuConfirm);
                activate(selectedItem);
            }
        }
        wasActiveLastFrame = active;
    }

    for (const auto& shader : { menuFont->getShader(), Quad::getShader() }) {
        shader->bind();
        shader->setMat4("projection", orthoCamera->getProjection());
        shader->unbind();
    }

    const auto width  = static_cast<float>(orthoCamera->getWidth());
    const auto height = static_cast<float>(orthoCamera->getHeight());
    quad->render({ 0.F, 0.F }, { width, height }, backgroundColor);

    for (const auto& [item, action, label] : bindingRows) {
        const auto [x, y, w, h] = rowLayout(item);
        renderRowBackground(x, y, w, h, item);

        const bool capturing = rebindingItem == item;
        const auto value =
            capturing ? std::string(captureMessage) :
                        sponge::platform::glfw::core::InputManager::keyLabel(
                            mgr.getPrimaryKey(action));
        const auto valueWidth =
            static_cast<float>(menuFont->getLength(value, fontSize));
        const float textY = std::floor(
            y + (h - static_cast<float>(menuFont->getHeight(fontSize))) / 2.F);

        menuFont->beginPass(fontSize);
        menuFont->render(label, { x + textMarginLeft, textY }, textColor);
        menuFont->render(value, { x + w - textMarginLeft - valueWidth, textY },
                         capturing ? captureColor : textColor);
        menuFont->endPass();
    }

    for (const auto item : { KeyMapItem::ResetDefaults, KeyMapItem::Return }) {
        const auto [x, y, w, h] = rowLayout(item);
        auto* const button      = buttonFor(item);
        button->setPosition({ x, y }, { x + w, y + h });
        ui::updateMenuButtonVisuals(button, selectedItem == item,
                                    textHoverColor);
        UNUSED(button->onUpdate(elapsedTime));
    }

    // the strip lines up with the rows, so it starts at the first row's edge
    const auto [tabX, tabY, tabW, tabH] = rowLayout(KeyMapItem::MoveForward);
    ui::renderTabBar(ui::OptionTab::Keyboard, menuFont,
                     orthoCamera->getProjection(), width, tabX);

    ui::renderKeyHints(keyMapKeyHints, menuFont, orthoCamera->getProjection(),
                       width, height);

    if (!isActive()) {
        wasActiveLastFrame    = false;
        waitForConfirmRelease = false;
    }

    return true;
}

void KeyMapLayer::renderRowBackground(const float x, const float y,
                                      const float w, const float h,
                                      const KeyMapItem item) const {
    const bool isSelected = selectedItem == item;
    const bool isHovered  = hoveredItem == item;
    quad->render({ x, y }, { x + w, y + h },
                 isSelected ? textHoverColor :
                 isHovered  ? hoverColor :
                              buttonColor,
                 cornerRadius, isSelected ? selectedBorderWidth : 0.F,
                 glm::vec4{ 1.F });
}

std::tuple<float, float, float, float>
    KeyMapLayer::rowLayout(const KeyMapItem item) {
    const auto [rootX, rootY, rootW, rootH] = ui::getNodeLayout(
        rootNode, 0.F,
        ui::tabBarHeight(static_cast<float>(orthoCamera->getWidth())));
    const auto [menuX, menuY, menuW, menuH] =
        ui::getNodeLayout(menuNode, rootX, rootY);
    const auto [bgX, bgY, bgW, bgH] =
        ui::getNodeLayout(menuBackgroundNode, menuX, menuY);
    return ui::getNodeLayout(rowNodes[static_cast<size_t>(item)], bgX, bgY);
}

void KeyMapLayer::activate(const KeyMapItem item) {
    switch (item) {
        case KeyMapItem::ResetDefaults:
            inputManager().requestResetBindings();
            return;
        case KeyMapItem::Return:
            close();
            return;
        default:
            break;
    }

    rebindingItem = item;
    inputManager().requestRebind(bindingRows[static_cast<size_t>(item)].action);
}

void KeyMapLayer::close() {
    clearHoveredItems();
    selectedItem = KeyMapItem::MoveForward;
    setActive(false);
}

void KeyMapLayer::recalculateLayout(const float width, const float height) {
    // leave room for the tab bar along the top and the key hint bar along the
    // bottom
    const auto usableHeight =
        std::max(0.F, ui::heightWithoutKeyHints(width, height) -
                          ui::tabBarHeight(width));
    for (auto* const row : rowNodes) {
        ui::setMenuRowHeight(row, width);
    }

    // this menu fills its column, so keep Reset to Defaults off Return
    YGNodeStyleSetMargin(rowNodes[+KeyMapItem::ResetDefaults], YGEdgeBottom,
                         ui::menuRowHeight(width) * 0.4F);
    ui::pinMenuRowToBottom(rowNodes[+KeyMapItem::Return], width);
    YGNodeStyleSetWidth(rootNode, width);
    YGNodeStyleSetHeight(rootNode, usableHeight);
    YGNodeCalculateLayout(rootNode, width, usableHeight, YGDirectionLTR);
}

bool KeyMapLayer::onMouseButtonPressed(const MouseButtonPressedEvent& event) {
    if (event.getMouseButton() != sponge::input::MouseButton::Button0 ||
        rebindingItem) {
        return false;
    }

    auto [mouseX, mouseY] =
        sponge::platform::glfw::core::Application::get().getMousePosition();

    const auto [tabX, tabY, tabW, tabH] = rowLayout(KeyMapItem::MoveForward);
    const auto clickedTab =
        ui::tabBarHitTest(menuFont, static_cast<float>(orthoCamera->getWidth()),
                          tabX, { mouseX, mouseY });
    if (clickedTab) {
        clearHoveredItems();
        ui::showOptionTab(*clickedTab);
        return true;
    }

    for (size_t i = 0; i < rowCount; i++) {
        const auto item         = static_cast<KeyMapItem>(i);
        const auto [x, y, w, h] = rowLayout(item);
        if (contains(x, y, w, h, mouseX, mouseY)) {
            selectedItem = item;
            activate(item);
            break;
        }
    }

    return true;
}

bool KeyMapLayer::onMouseMoved(const MouseMovedEvent& event) {
    const auto pos = glm::vec2{ event.getX(), event.getY() };

    ui::updateButtonHover(resetButton.get(), pos);
    ui::updateButtonHover(returnButton.get(), pos);

    std::optional<KeyMapItem> nextHover;
    for (const auto& [item, action, label] : bindingRows) {
        const auto [x, y, w, h] = rowLayout(item);
        if (contains(x, y, w, h, pos.x, pos.y)) {
            nextHover = item;
            break;
        }
    }
    if (nextHover && nextHover != hoveredItem) {
        ui::playHoverClick();
    }
    hoveredItem = nextHover;

    return true;
}

bool KeyMapLayer::onWindowResize(const WindowResizeEvent& event) {
    orthoCamera->setWidthAndHeight(event.getWidth(), event.getHeight());

    const auto width  = static_cast<float>(event.getWidth());
    const auto height = static_cast<float>(event.getHeight());
    recalculateLayout(width, height);

    const auto newFontSize = ui::menuFontSizeForWidth(event.getWidth());
    if (newFontSize != fontSize) {
        fontSize = newFontSize;
        resetButton->setFontSize(fontSize);
        returnButton->setFontSize(fontSize);
    }

    return false;
}

void KeyMapLayer::clearHoveredItems() {
    resetButton->setHover(false);
    returnButton->setHover(false);
    hoveredItem = std::nullopt;
}
}  // namespace game::layer
