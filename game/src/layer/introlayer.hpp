#pragma once

#include "layer/buttonmenulayer.hpp"
#include "ui/menulayout.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace game::layer {

enum class IntroMenuItem : uint8_t { NewGame = 0, Options, Quit, Count };

// Buttons follow IntroMenuItem order, so the enum is the button index.
inline constexpr std::array<std::string_view, 3> introMenuLabels = {
    "New Game",
    "Options",
    "Quit",
};

inline constexpr ui::MenuLayoutSpec introMenuLayout = {
    .backgroundWidthPercent = 30.F,
};

inline constexpr std::array<ui::KeyHint, 2> introKeyHints = {
    ui::KeyHint{ "keyboard_arrows_vertical", "xbox_dpad_vertical", "Navigate" },
    ui::KeyHint{ "keyboard_enter", "xbox_button_a", "Select" },
};

class IntroLayer final : public ButtonMenuLayer {
public:
    IntroLayer();

    bool onUpdate(double elapsedTime) override;

    void beginFadeIn(double duration);

private:
    bool   isFadingIn        = false;
    double fadeInDuration    = 0.0;
    double fadeInAccumulator = 0.0;

protected:
    void onSelect(size_t item) override;

    bool takesInput() const override;
};
}  // namespace game::layer
