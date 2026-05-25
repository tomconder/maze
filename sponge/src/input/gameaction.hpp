#pragma once
#include <cstdint>

namespace sponge::input {

enum class GameAction : uint8_t {
    MenuUp,
    MenuDown,
    MenuLeft,
    MenuRight,
    MenuConfirm,
    MenuBack,
    MoveForward,
    MoveBack,
    MoveLeft,
    MoveRight,
    LookHorizontal,
    LookVertical,
    Pause,
    ExitGame,
    ToggleFullscreen,
    ToggleDebugUI,
    Count
};

constexpr int operator+(const GameAction a) noexcept {
    return static_cast<int>(a);
}

}  // namespace sponge::input
