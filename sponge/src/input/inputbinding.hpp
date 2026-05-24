#pragma once
#include "input/gameaction.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace sponge::input {

enum class BindingType : uint8_t {
    Key,
    MouseButton,
    MouseAxisX,
    MouseAxisY,
    GamepadButton,
    GamepadAxis
};

struct InputBinding {
    BindingType type      = BindingType::Key;
    int         rawCode   = 0;
    float       axisScale = 1.f;
    float       deadzone  = 0.f;
};

using BindingList = std::vector<InputBinding>;
using BindingMap  = std::array<BindingList, +GameAction::Count>;

}  // namespace sponge::input
