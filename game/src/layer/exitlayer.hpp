#pragma once

#include "layer/buttonmenulayer.hpp"
#include "ui/menulayout.hpp"

#include <cstddef>
#include <cstdint>

namespace game::layer {

enum class ExitMenuItem : uint8_t {
    Continue = 0,
    Options,
    ReturnToMenu,
    Exit,
    Count,
};

inline constexpr ui::MenuLayoutSpec exitMenuLayout = {
    .backgroundWidthPercent = 45.F,
    .rootWidthScale         = 0.54F,
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
