#pragma once

#include "layer/buttonmenulayer.hpp"
#include "ui/menulayout.hpp"

#include <cstddef>
#include <cstdint>

namespace game::layer {

enum class IntroMenuItem : uint8_t { NewGame = 0, Options, Quit, Count };

inline constexpr ui::MenuLayoutSpec introMenuLayout = {
    .backgroundWidthPercent = 30.F,
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
