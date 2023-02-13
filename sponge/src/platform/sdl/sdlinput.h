#pragma once

#include "core/input.h"

#include <unordered_map>

#include "glm/vec2.hpp"

class SDLInput : public Input {
   public:
    SDLInput();

    void beginFrame();
    void keyDown(const KeyPressedEvent &event) override;
    void keyUp(const KeyReleasedEvent &event) override;

    bool isKeyHeld(KeyCode key) override;
    bool wasKeyPressed(KeyCode key) override;
    bool wasKeyReleased(KeyCode key) override;

    void mouseButtonDown(const MouseButtonPressedEvent &event) override;
    void mouseButtonUp(const MouseButtonReleasedEvent &event) override;

    // TODO change this to take button as a parameter
    bool isButtonPressed() const override;
    void mouseMove(const MouseMovedEvent &event) override;
    void mouseScroll(const MouseScrolledEvent &event) override;

    glm::vec2 getMoveDelta() const override {
        return moveDelta;
    }
    glm::vec2 getScrollDelta() override;

    KeyCode mapScanCodeToKeyCode(const SDL_Scancode& scancode);
    MouseCode mapMouseButton(uint8_t index);

   private:
    std::unordered_map<KeyCode, bool> heldKeys;
    std::unordered_map<KeyCode, bool> pressedKeys;
    std::unordered_map<KeyCode, bool> releasedKeys;
    std::unordered_map<SDL_Scancode, KeyCode> keymap;

    bool buttonPressed = false;
    glm::vec2 moveDelta = { 0.f, 0.f };
    glm::vec2 scrollDelta = { 0.f, 0.f };
    float lastScrollX = 0.f;
    void initializeKeyMap();
};
