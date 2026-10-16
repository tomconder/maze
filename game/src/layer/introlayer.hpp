#pragma once

#include "layer/buttonmenulayer.hpp"
#include "ui/menulayout.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace game::layer {

enum class IntroMenuItem : uint8_t { NewGame = 0, Options, Quit, Count };

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
