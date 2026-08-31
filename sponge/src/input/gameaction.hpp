#pragma once
#include <array>
#include <cstdint>
#include <string_view>

namespace sponge::input {

enum class GameAction : uint8_t {
    MenuUp,
    MenuDown,
    MenuLeft,
    MenuRight,
    MenuConfirm,
    MenuBack,
    TabPrev,
    TabNext,
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
    Count,
};

constexpr int operator+(const GameAction a) noexcept {
    return static_cast<int>(a);
}

// Stable identifiers, indexed by GameAction. Used to build settings keys for
// rebound keys, so renaming one discards the player's binding.
constexpr std::array<std::string_view, +GameAction::Count> actionNames{
    "menuUp",           "menuDown",      "menuLeft", "menuRight",
    "menuConfirm",      "menuBack",      "tabPrev",  "tabNext",
    "moveForward",      "moveBack",      "moveLeft", "moveRight",
    "lookHorizontal",   "lookVertical",  "pause",    "exitGame",
    "toggleFullscreen", "toggleDebugUI",
};

// A missing name would build settings keys of "input.", so catch it here
// rather than in the settings file.
static_assert([] {
    for (const auto& name : actionNames) {
        if (name.empty()) {
            return false;
        }
    }
    return true;
}());

}  // namespace sponge::input
