#pragma once

namespace game::ui {

// Keeps a confirm press that opened a menu from also choosing an item: the
// menu ignores confirm until the key is let go once.
struct ConfirmGate {
    // True when the menu took input on the last frame.
    bool wasActive = false;
    bool waiting   = false;

    // Call once a frame. Returns true on the frame the menu starts to take
    // input.
    bool update(const bool takesInput, const bool confirmHeld) {
        const bool opened = takesInput && !wasActive;
        if (opened) {
            waiting = confirmHeld;
        } else if (waiting && !confirmHeld) {
            waiting = false;
        }
        return opened;
    }

    // Call at the end of the frame's input handling.
    void end(const bool takesInput) {
        wasActive = takesInput;
    }

    bool ready() const {
        return !waiting;
    }

    void reset() {
        wasActive = false;
        waiting   = false;
    }
};

}  // namespace game::ui
