#pragma once

#include "event/applicationevent.hpp"
#include "event/event.hpp"
#include "event/mouseevent.hpp"
#include "layer/layer.hpp"
#include "ui/confirmdialog.hpp"

#include <functional>
#include <memory>
#include <string_view>

namespace game::layer {

inline constexpr std::string_view displayChangeTitle =
    "Confirm Graphical Changes";
inline constexpr std::string_view displayChangeMessage =
    "Keep the changes to your graphical settings?";

// A yes or no prompt, on top of every other layer. It is modal: while it is
// active it takes the mouse events first, and the layers under it skip their
// keys (see Maze::getConfirmLayer()).
class ConfirmLayer final : public sponge::layer::Layer {
public:
    ConfirmLayer();

    void onAttach() override;

    void onEvent(sponge::event::Event& event) override;

    bool onUpdate(double elapsedTime) override;

    // Opens the prompt. onResult runs once with Accepted or Cancelled, after
    // the layer has gone inactive. The countdown running out is Cancelled;
    // seconds <= 0 has no countdown. defaultAccept picks the selected button.
    void show(std::string_view title, std::string_view message,
              std::function<void(ui::ConfirmResult)> callback, float seconds,
              bool defaultAccept = true);

private:
    std::unique_ptr<ui::ConfirmDialog>     dialog;
    std::function<void(ui::ConfirmResult)> onResult;
    bool                                   wasActiveLastFrame = false;

    void finish(ui::ConfirmResult result);

    bool onMouseButtonPressed(
        const sponge::event::MouseButtonPressedEvent& event);

    bool onMouseMoved(const sponge::event::MouseMovedEvent& event);

    static bool onWindowResize(const sponge::event::WindowResizeEvent& event);
};

}  // namespace game::layer
