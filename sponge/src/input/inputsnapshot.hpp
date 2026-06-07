// sponge/src/input/inputsnapshot.hpp
#pragma once
#include "input/activedevice.hpp"
#include "input/gameaction.hpp"

#include <array>
#include <string_view>

namespace sponge::input {

struct InputSnapshot {
    std::array<bool, +GameAction::Count>  active{};
    std::array<bool, +GameAction::Count>  held{};
    std::array<float, +GameAction::Count> axis{};

    ActiveDevice activeDevice     = ActiveDevice::KeyboardMouse;
    bool         gamepadConnected = false;
    int          gamepadSlot      = -1;

    [[nodiscard]] bool isActive(const GameAction a) const noexcept {
        return active[+a];
    }
    [[nodiscard]] bool isHeld(const GameAction a) const noexcept {
        return held[+a];
    }
    [[nodiscard]] float getAxis(const GameAction a) const noexcept {
        return axis[+a];
    }
    [[nodiscard]] std::string_view getPromptText(GameAction a) const noexcept;
};

}  // namespace sponge::input
