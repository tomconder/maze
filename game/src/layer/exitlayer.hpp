#pragma once

#include "layer/buttonmenulayer.hpp"
#include "ui/menulayout.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace game::layer {

enum class ExitMenuItem : uint8_t {
    Continue = 0,
    Options,
    ReturnToMenu,
    Exit,
    Count,
};

// Buttons follow ExitMenuItem order, so the enum is the button index.
inline constexpr std::array<std::string_view, 4> exitMenuLabels = {
    "Continue",
    "Options",
    "Return to Menu",
    "Exit the Game",
};

inline constexpr ui::MenuLayoutSpec exitMenuLayout = {
    .backgroundWidthPercent = 45.F,
    .rootWidthScale         = 0.54F,
};

inline constexpr std::array<ui::KeyHint, 3> exitKeyHints = {
    ui::KeyHint{ "keyboard_arrows_vertical", "xbox_dpad_vertical", "Navigate" },
    ui::KeyHint{ "keyboard_enter", "xbox_button_a", "Select" },
    ui::KeyHint{ "keyboard_escape", "xbox_button_b", "Back" },
};

class ExitLayer final : public ButtonMenuLayer {
public:
    ExitLayer();

protected:
    void onSelect(size_t item) override;

    bool onBack() override;

private:
    void resumeGame();
};
}  // namespace game::layer
