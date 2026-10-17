#pragma once

#include "event/applicationevent.hpp"
#include "event/event.hpp"
#include "event/mouseevent.hpp"
#include "input/inputsnapshot.hpp"
#include "layer/layer.hpp"
#include "platform/opengl/scene/bitmapfont.hpp"
#include "platform/opengl/scene/quad.hpp"
#include "scene/orthocamera.hpp"
#include "ui/confirmgate.hpp"
#include "ui/keyhints.hpp"
#include "ui/menulayout.hpp"
#include "ui/menuscroll.hpp"
#include "ui/tabbar.hpp"

#include <glm/glm.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <tuple>

namespace game::layer {

// The hints of the tabs whose rows change a value.
inline constexpr std::array<ui::KeyHint, 4> adjustKeyHints = {
    ui::KeyHint{ "keyboard_arrows_vertical", "xbox_dpad_vertical", "Navigate" },
    ui::KeyHint{ "keyboard_arrows_horizontal", "xbox_dpad_horizontal",
                 "Change" },
    ui::KeyHint{ "keyboard_enter", "xbox_button_a", "Select" },
    ui::KeyHint{ "keyboard_escape", "xbox_button_b", "Back" },
};

// No spacer above: the rows start under the tab bar.
inline constexpr ui::MenuLayoutSpec scrollMenuLayout = {
    .backgroundWidthPercent = 45.F,
    .titleFlexGrow          = 0.F,
    .optionTab              = true,
};

// What differs between two option tabs.
struct ScrollMenuStyle {
    std::string                  name;
    ui::OptionTab                tab;
    size_t                       rowCount;
    std::span<const ui::KeyHint> keyHints;
    // Index in keyHints of a hint that can be clicked; see onActionHint().
    std::optional<size_t> actionHint = std::nullopt;
    // The row selected when the tab opens and after it closes.
    size_t firstRow = 0;
};

// One tab of the options screen: rows under a tab bar, scrolling when they do
// not fit. The base owns chrome, selection, scrolling and the mouse; a subclass
// draws a row's content and reacts to keys and clicks on it.
class ScrollMenuLayer : public sponge::layer::Layer {
public:
    void onAttach() override;

    void onDetach() override;

    void onEvent(sponge::event::Event& event) override;

    bool onUpdate(double elapsedTime) override;

    bool isMenu() const override {
        return true;
    }

protected:
    explicit ScrollMenuLayer(ScrollMenuStyle menuStyle);

    // Draws a row's content over its highlight, inside the scroll clip. y is
    // where the row starts as drawn.
    virtual void renderRow(size_t row, float y) = 0;

    // False for a row that is only a label, such as a heading: it is not
    // selected, hovered, clicked or highlighted, and the selection hops over
    // it. renderRow() still draws it.
    virtual bool isSelectable(size_t /*row*/) const {
        return true;
    }

    // Runs when the tab opens, before it scrolls to the selected row.
    virtual void onOpened() {}

    // Runs when the tab closes, after the selection went back to firstRow.
    virtual void onClosed() {}

    // False while the screen must ignore keys, clicks and the wheel, such as
    // while a key is being captured.
    virtual bool takesInput() const {
        return true;
    }

    // Runs once a frame after the selection moved, with the selected row.
    virtual void onRowKeys(size_t /*row*/,
                           const sponge::input::InputSnapshot& /*input*/) {}

    // The Enter or gamepad confirm on the selected row. Return true when it
    // was used, so no other layer sees it.
    virtual bool onConfirm(size_t /*row*/) {
        return false;
    }

    virtual void onActionHint() {}

    // A press on a row; rowRect(row) is its rectangle. Return true to follow
    // the mouse with onRowDrag() until the button is released.
    virtual bool onRowPressed(size_t /*row*/, const glm::vec2& /*mouse*/) {
        return false;
    }

    virtual void onRowDrag(size_t /*row*/, const glm::vec2& /*mouse*/) {}

    virtual void onRowReleased(size_t /*row*/) {}

    virtual void onFontSizeChanged() {}

    // Runs after every window size change, once the layout follows it.
    virtual void onResized(uint32_t /*width*/, uint32_t /*height*/) {}

    // x, y, width, height of a row in window pixels.
    std::tuple<float, float, float, float> rowRect(size_t row) const;

    // The Yoga node of a row. A widget adds its parts to it; call
    // recalculateLayout() once they are there.
    YGNodeRef rowNode(size_t row) const;

    void recalculateLayout();

    void clearHoveredItems();

    const std::shared_ptr<sponge::platform::opengl::scene::BitmapFont>&
        menuFont() const {
        return font;
    }

    uint32_t menuFontSize() const {
        return fontSize;
    }

private:
    ScrollMenuStyle menuStyle;

    std::shared_ptr<sponge::platform::opengl::scene::BitmapFont> font;
    std::shared_ptr<scene::OrthoCamera>                          camera;
    std::unique_ptr<sponge::platform::opengl::scene::Quad>       quad;
    std::unique_ptr<ui::MenuLayout>                              layout;
    ui::MenuScroll                                               scroll;

    uint32_t fontSize = 0;

    size_t                selected = 0;
    std::optional<size_t> hovered;
    std::optional<size_t> hoveredHint;
    // Row following the mouse; set by a press, cleared on release.
    std::optional<size_t> dragging;

    ui::ConfirmGate confirmGate;

    void handleInput();

    void close();

    // Hint under the mouse, by index in the style's key hints.
    std::optional<size_t> hintAt(const glm::vec2& pos) const;

    void renderRowBackground(size_t row, float x, float y, float w,
                             float h) const;

    void followSelection();

    bool onMouseButtonPressed(
        const sponge::event::MouseButtonPressedEvent& event);

    bool onMouseMoved(const sponge::event::MouseMovedEvent& event);

    bool onMouseButtonReleased(
        const sponge::event::MouseButtonReleasedEvent& event);

    bool onMouseScrolled(const sponge::event::MouseScrolledEvent& event);

    bool onWindowResize(const sponge::event::WindowResizeEvent& event);
};

}  // namespace game::layer
