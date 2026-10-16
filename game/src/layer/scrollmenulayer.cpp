#include "layer/scrollmenulayer.hpp"

#include "input/gameaction.hpp"
#include "input/inputcontext.hpp"
#include "input/mousecode.hpp"
#include "platform/glfw/core/application.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"
#include "ui/button.hpp"
#include "ui/menufontsize.hpp"
#include "ui/menuselection.hpp"

#include <glm/glm.hpp>

#include <algorithm>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

namespace {
constexpr std::string_view fontName = "inter";
constexpr std::string_view fontPath = "/fonts/inter.ktx2";

constexpr glm::vec4 backgroundColor = { 0.F, 0.F, 0.F, 1.F };
constexpr glm::vec4 rowColor        = { 0.F, 0.F, 0.F, 0.F };
constexpr glm::vec4 hoverColor      = { 0.84F, 0.84F, 0.84F, 0.14F };
constexpr glm::vec4 selectedColor   = { 0.84F, 0.04F, 0.04F, 0.14F };

constexpr float cornerRadius        = 12.F;
constexpr float selectedBorderWidth = 3.F;

bool contains(const float x, const float y, const float w, const float h,
              const float px, const float py) {
    return px >= x && px <= x + w && py >= y && py <= y + h;
}
}  // namespace

namespace game::layer {
using sponge::event::Event;
using sponge::event::EventDispatcher;
using sponge::event::MouseButtonPressedEvent;
using sponge::event::MouseButtonReleasedEvent;
using sponge::event::MouseMovedEvent;
using sponge::event::MouseScrolledEvent;
using sponge::event::WindowResizeEvent;
using sponge::platform::glfw::core::Application;
using sponge::platform::opengl::renderer::AssetManager;
using sponge::platform::opengl::scene::FontCreateInfo;
using sponge::platform::opengl::scene::Quad;

ScrollMenuLayer::ScrollMenuLayer(ScrollMenuStyle style) :
    Layer(style.name),
    menuStyle(std::move(style)),
    selected(menuStyle.firstRow) {}

void ScrollMenuLayer::onAttach() {
    font     = AssetManager::createFont(FontCreateInfo{
        .name = std::string(fontName),
        .path = std::string(fontPath),
    });
    camera   = scene::menuOrthoCamera();
    quad     = std::make_unique<Quad>();
    fontSize = ui::menuFontSizeForWidth(camera->getWidth());

    for (const auto& shader : { font->getShader(), Quad::getShader() }) {
        shader->setMat4("projection", camera->getProjection());
    }

    layout = std::make_unique<ui::MenuLayout>(
        menuStyle.rowCount, scrollMenuLayout, menuStyle.keyHints);
    recalculateLayout();
}

void ScrollMenuLayer::onDetach() {
    layout.reset();
}

void ScrollMenuLayer::onEvent(Event& event) {
    EventDispatcher dispatcher(event);

    dispatcher.dispatch<MouseButtonPressedEvent>(
        [this](const MouseButtonPressedEvent& mouseEvent) {
            return isActive() ? onMouseButtonPressed(mouseEvent) : false;
        });
    dispatcher.dispatch<MouseButtonReleasedEvent>(
        [this](const MouseButtonReleasedEvent& mouseEvent) {
            return isActive() ? onMouseButtonReleased(mouseEvent) : false;
        });
    dispatcher.dispatch<MouseScrolledEvent>(
        [this](const MouseScrolledEvent& scrollEvent) {
            return isActive() ? onMouseScrolled(scrollEvent) : false;
        });
    dispatcher.dispatch<MouseMovedEvent>(
        [this](const MouseMovedEvent& mouseMovedEvent) {
            return isActive() ? onMouseMoved(mouseMovedEvent) : false;
        });
    dispatcher.dispatch<WindowResizeEvent>(
        [this](const WindowResizeEvent& windowResizeEvent) {
            return onWindowResize(windowResizeEvent);
        });
}

void ScrollMenuLayer::handleInput() {
    using sponge::input::GameAction;
    auto& mgr = Application::get().getInputManager();
    mgr.setActiveContext(sponge::input::InputContext::Menu);

    const bool  takes = takesInput();
    const auto& input = mgr.getSnapshot();
    if (confirmGate.update(takes, input.isHeld(GameAction::MenuConfirm))) {
        onOpened();
        followSelection();
        // open at the right row, not easing in from the last visit
        scroll.snap();
    }

    if (confirmGate.wasActive && takes) {
        if (ui::stepSelection(input, selected, menuStyle.rowCount)) {
            // hop over labels in the direction of travel
            const bool up = input.isActive(GameAction::MenuUp) &&
                            !input.isActive(GameAction::MenuDown);
            while (!isSelectable(selected)) {
                selected = (selected + (up ? menuStyle.rowCount - 1 : 1)) %
                           menuStyle.rowCount;
            }
            ui::playHoverClick();
            followSelection();
        }

        onRowKeys(selected, input);

        if (input.isActive(GameAction::TabNext)) {
            mgr.consumeActive(GameAction::TabNext);
            clearHoveredItems();
            ui::showOptionTab(ui::cycleTab(menuStyle.tab, 1));
        }
        if (input.isActive(GameAction::TabPrev)) {
            mgr.consumeActive(GameAction::TabPrev);
            clearHoveredItems();
            ui::showOptionTab(ui::cycleTab(menuStyle.tab, -1));
        }
        if (input.isActive(GameAction::MenuBack)) {
            mgr.consumeActive(GameAction::MenuBack);
            close();
        }
        if (confirmGate.ready() && input.isActive(GameAction::MenuConfirm) &&
            onConfirm(selected)) {
            mgr.consumeActive(GameAction::MenuConfirm);
        }
    }
    confirmGate.end(takes);
}

bool ScrollMenuLayer::onUpdate(const double elapsedTime) {
    handleInput();

    for (const auto& shader : { font->getShader(), Quad::getShader() }) {
        shader->setMat4("projection", camera->getProjection());
    }

    const auto width  = static_cast<float>(camera->getWidth());
    const auto height = static_cast<float>(camera->getHeight());
    scroll.step(static_cast<float>(elapsedTime));
    quad->render({ 0.F, 0.F }, { width, height }, backgroundColor);

    scroll.beginClip(height);
    for (size_t row = 0; row < menuStyle.rowCount; row++) {
        if (!scroll.shows(row)) {
            continue;
        }
        const auto [x, y, w, h] = rowRect(row);
        if (isSelectable(row)) {
            renderRowBackground(row, x, y, w, h);
        }
        renderRow(row, x, y, w, h);
    }
    scroll.endClip();

    scroll.render(*quad);

    // the strip lines up with the rows, so it starts at the first row's edge
    ui::renderTabBar(menuStyle.tab, font, camera->getProjection(), width,
                     layout->tabBarRects());

    // the prompts change size with the device
    if (ui::usingGamepad() != hintsForGamepad) {
        recalculateLayout();
    }
    ui::renderKeyHints(menuStyle.keyHints, font, camera->getProjection(), width,
                       layout->keyHintRects(), hoveredHint);

    if (!isActive()) {
        confirmGate.reset();
        dragging = std::nullopt;
    }

    return true;
}

void ScrollMenuLayer::renderRowBackground(const size_t row, const float x,
                                          const float y, const float w,
                                          const float h) const {
    const bool isSelected = selected == row;
    const bool isHovered  = hovered == row;
    quad->render({ x, y }, { x + w, y + h },
                 isSelected ? selectedColor :
                 isHovered  ? hoverColor :
                              rowColor,
                 cornerRadius, isSelected ? selectedBorderWidth : 0.F,
                 glm::vec4{ 1.F });
}

std::tuple<float, float, float, float>
    ScrollMenuLayer::rowRect(const size_t row) const {
    const auto [x, y, w, h] = layout->rowRect(row);
    return { x, y - scroll.pixelOffset(), w, h };
}

std::optional<size_t> ScrollMenuLayer::hintAt(const glm::vec2& pos) const {
    return ui::hitTestKeyHints(layout->keyHintRects(), pos);
}

void ScrollMenuLayer::close() {
    clearHoveredItems();
    selected = menuStyle.firstRow;
    onClosed();
    setActive(false);
}

void ScrollMenuLayer::clearHoveredItems() {
    scroll.release();
    hoveredHint = std::nullopt;
    hovered     = std::nullopt;
}

void ScrollMenuLayer::recalculateLayout() {
    const auto width   = static_cast<float>(camera->getWidth());
    const auto measure = ui::fontMeasure(font);
    hintsForGamepad    = ui::usingGamepad();
    layout->setTabLabelSizes(ui::tabLabelSizes(width, measure));
    layout->setKeyHintSizes(
        ui::keyHintSizes(menuStyle.keyHints, width, hintsForGamepad, measure));
    scroll.layout(*layout, width, static_cast<float>(camera->getHeight()));
}

void ScrollMenuLayer::followSelection() {
    // keep the label above a section's first row in view too
    if (selected > 0 && !isSelectable(selected - 1)) {
        scroll.ensureVisible(selected - 1);
    }
    scroll.ensureVisible(selected);
}

bool ScrollMenuLayer::onMouseScrolled(const MouseScrolledEvent& event) {
    if (dragging || !takesInput()) {
        return true;
    }
    if (scroll.wheel(event.getYOffset())) {
        // no mouse move follows, so the old hover points at the wrong row
        hovered = std::nullopt;
    }
    return true;
}

bool ScrollMenuLayer::onMouseButtonPressed(
    const MouseButtonPressedEvent& event) {
    if (event.getMouseButton() != sponge::input::MouseButton::Button0 ||
        !takesInput()) {
        return false;
    }

    const auto [mouseX, mouseY] = Application::get().getMousePosition();

    const auto clickedTab = ui::tabBarHitTest(
        layout->tabBarRects(), static_cast<float>(camera->getWidth()),
        { mouseX, mouseY });
    if (clickedTab) {
        clearHoveredItems();
        ui::showOptionTab(*clickedTab);
        return true;
    }

    if (menuStyle.actionHint &&
        hintAt({ mouseX, mouseY }) == menuStyle.actionHint) {
        onActionHint();
        return true;
    }

    if (scroll.press({ mouseX, mouseY })) {
        return true;
    }

    for (size_t row = 0; row < menuStyle.rowCount; row++) {
        const auto [x, y, w, h] = rowRect(row);
        if (!isSelectable(row) || !scroll.canHit(row, mouseY) ||
            !contains(x, y, w, h, mouseX, mouseY)) {
            continue;
        }
        selected = row;
        if (onRowPressed(row, { mouseX, mouseY })) {
            dragging = row;
        }
        break;
    }

    return true;
}

bool ScrollMenuLayer::onMouseMoved(const MouseMovedEvent& event) {
    const auto pos = glm::vec2{ event.getX(), event.getY() };

    const auto hint = hintAt(pos);
    hoveredHint = hint && hint == menuStyle.actionHint ? hint : std::nullopt;

    scroll.drag(pos.y);
    if (scroll.isDragging()) {
        return true;
    }

    if (dragging) {
        onRowDrag(*dragging, pos);
        return true;
    }

    std::optional<size_t> nextHover;
    for (size_t row = 0; row < menuStyle.rowCount; row++) {
        const auto [x, y, w, h] = rowRect(row);
        if (isSelectable(row) && scroll.canHit(row, pos.y) &&
            contains(x, y, w, h, pos.x, pos.y)) {
            nextHover = row;
            break;
        }
    }
    if (nextHover && nextHover != hovered) {
        ui::playHoverClick();
    }
    hovered = nextHover;

    return true;
}

bool ScrollMenuLayer::onMouseButtonReleased(
    const MouseButtonReleasedEvent& event) {
    if (event.getMouseButton() != sponge::input::MouseButton::Button0) {
        return false;
    }
    if (scroll.isDragging()) {
        scroll.release();
        return true;
    }
    if (!dragging) {
        return false;
    }

    onRowReleased(*dragging);
    dragging = std::nullopt;
    return true;
}

bool ScrollMenuLayer::onWindowResize(const WindowResizeEvent& event) {
    camera->setWidthAndHeight(event.getWidth(), event.getHeight());
    recalculateLayout();
    followSelection();

    const auto newFontSize = ui::menuFontSizeForWidth(event.getWidth());
    if (newFontSize != fontSize) {
        fontSize = newFontSize;
        onFontSizeChanged();
    }
    onResized(event.getWidth(), event.getHeight());

    return false;
}
}  // namespace game::layer
