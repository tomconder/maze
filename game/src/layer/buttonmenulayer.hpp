#pragma once

#include "event/applicationevent.hpp"
#include "event/event.hpp"
#include "event/mouseevent.hpp"
#include "layer/layer.hpp"
#include "platform/opengl/scene/bitmapfont.hpp"
#include "platform/opengl/scene/quad.hpp"
#include "scene/orthocamera.hpp"
#include "ui/button.hpp"
#include "ui/confirmgate.hpp"
#include "ui/keyhints.hpp"
#include "ui/menulayout.hpp"

#include <glm/glm.hpp>

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace game::layer {

// What differs between two menus built from a column of buttons.
struct ButtonMenuStyle {
    std::string                       name;
    std::span<const std::string_view> labels;
    std::span<const ui::KeyHint>      keyHints;
    ui::MenuLayoutSpec                layout;
    glm::vec4                         buttonColor;
    glm::vec4                         selectedColor;
    // Drawn over the whole window first; empty leaves what is below visible.
    std::optional<glm::vec4> backgroundColor;
    // A modal menu takes every mouse event, so layers below see none.
    bool modal = false;
};

// A menu of left-aligned buttons with keyboard, gamepad and mouse selection.
// A subclass gives the style and acts on a chosen item.
class ButtonMenuLayer : public sponge::layer::Layer {
public:
    void onAttach() override;

    void onDetach() override;

    void onEvent(sponge::event::Event& event) override;

    bool onUpdate(double elapsedTime) override;

protected:
    explicit ButtonMenuLayer(ButtonMenuStyle menuStyle);

    // Runs when an item is chosen by Enter, the gamepad or a click.
    virtual void onSelect(size_t item) = 0;

    // Runs on the back action; return true when it was handled.
    virtual bool onBack() {
        return false;
    }

    // False while something covers the menu or it is not ready for input.
    virtual bool takesInput() const;

    // Selects the item and runs onSelect().
    void activate(size_t item);

    void clearHoveredItems() const;

private:
    ButtonMenuStyle menuStyle;

    std::shared_ptr<sponge::platform::opengl::scene::BitmapFont> font;
    std::shared_ptr<scene::OrthoCamera>                          camera;
    std::unique_ptr<sponge::platform::opengl::scene::Quad>       quad;
    std::unique_ptr<ui::MenuLayout>                              layout;
    std::vector<std::unique_ptr<ui::Button>>                     buttons;

    size_t          selected = 0;
    ui::ConfirmGate confirmGate;

    void handleInput();

    void recalculateLayout();

    bool onMouseButtonPressed(
        const sponge::event::MouseButtonPressedEvent& event);

    bool onMouseMoved(const sponge::event::MouseMovedEvent& event);

    bool onWindowResize(const sponge::event::WindowResizeEvent& event);
};
}  // namespace game::layer
