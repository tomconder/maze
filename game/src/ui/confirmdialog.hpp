#pragma once

#include "platform/glfw/core/inputmanager.hpp"
#include "platform/opengl/scene/bitmapfont.hpp"
#include "platform/opengl/scene/quad.hpp"
#include "ui/button.hpp"
#include "ui/confirmlayout.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace game::ui {

enum class ConfirmResult : uint8_t { Pending = 0, Accepted, Cancelled };

// A modal prompt with a title, one line of text, an optional countdown and an
// Accept and a Cancel button. The countdown running out counts as Cancelled.
class ConfirmDialog {
public:
    ConfirmDialog(
        std::shared_ptr<sponge::platform::opengl::scene::BitmapFont> font,
        uint32_t                                                     fontSize);

    // seconds <= 0 shows no countdown and never times out. defaultAccept
    // picks the selected button: Accept for a change that is easy to undo,
    // Cancel for one that is not.
    void open(std::string_view newTitle, std::string_view newMessage,
              float seconds, bool defaultAccept);

    void close() {
        opened = false;
    }

    bool isOpen() const {
        return opened;
    }

    void setFontSize(uint32_t size);

    // Left, Right, Confirm and Back. Consumes what it uses.
    ConfirmResult onInput(sponge::platform::glfw::core::InputManager& mgr);

    ConfirmResult onMousePressed(const glm::vec2& position);

    void onMouseMoved(const glm::vec2& position);

    // Counts down and draws over the whole window.
    ConfirmResult onUpdate(double elapsedTime, float width, float height);

private:
    std::string                                                  title;
    std::string                                                  message;
    std::shared_ptr<sponge::platform::opengl::scene::BitmapFont> font;
    uint32_t                                                     fontSize;

    std::unique_ptr<ConfirmLayout>                         layout;
    std::unique_ptr<sponge::platform::opengl::scene::Quad> quad;
    std::unique_ptr<Button>                                acceptButton;
    std::unique_ptr<Button>                                cancelButton;

    bool  opened       = false;
    bool  acceptChosen = true;
    bool  timed        = false;
    float remaining    = 0.F;
};

}  // namespace game::ui
