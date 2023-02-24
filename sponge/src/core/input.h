#pragma once

#include "event/keyevent.h"
#include "event/mouseevent.h"
#include "keycode.h"
#include "mousecode.h"

namespace Sponge {

class Input {
   public:
    virtual ~Input() = default;

    virtual void keyDown(const KeyPressedEvent &event) = 0;
    virtual void keyUp(const KeyReleasedEvent &event) = 0;

    virtual bool isKeyHeld(KeyCode key) = 0;
    virtual bool wasKeyPressed(KeyCode key) = 0;
    virtual bool wasKeyReleased(KeyCode key) = 0;

    virtual void mouseButtonDown(const MouseButtonPressedEvent &event) = 0;
    virtual void mouseButtonUp(const MouseButtonReleasedEvent &event) = 0;

    virtual bool isButtonPressed() const = 0;
    virtual void mouseMove(const MouseMovedEvent &event) = 0;
    virtual void mouseScroll(const MouseScrolledEvent &event) = 0;

    virtual glm::vec2 getMoveDelta() const = 0;
    virtual glm::vec2 getScrollDelta() = 0;
};

}  // namespace Sponge
