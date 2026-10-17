#pragma once

#include "layer/scrollmenulayer.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace game::layer {

enum class KeyMapItem : uint8_t {
    MoveForward = 0,
    MoveBack,
    MoveLeft,
    MoveRight,
    Pause,
    ToggleDebugUI,
    Count,
};

inline constexpr std::array<ui::KeyHint, 4> keyMapKeyHints = {
    ui::KeyHint{ "keyboard_arrows_vertical", "xbox_dpad_vertical", "Navigate" },
    ui::KeyHint{ "keyboard_enter", "xbox_button_a", "Rebind" },
    ui::KeyHint{ "keyboard_escape", "xbox_button_b", "Back" },
    ui::KeyHint{ "keyboard_r", "xbox_button_y", "Reset to Defaults", true },
};

class KeyMapLayer final : public ScrollMenuLayer {
public:
    KeyMapLayer();

    void onAttach() override;

    bool onUpdate(double elapsedTime) override;

private:
    // Row waiting for a key, while InputManager captures the next press.
    std::optional<size_t> rebindingRow;

    void startRebind(size_t row);

    void askReset();

protected:
    // The row being bound takes the input itself, and so does the prompt over
    // this screen.
    bool takesInput() const override;

    void renderRow(size_t row, float x, float y, float w, float h) override;

    void onFontSizeChanged() override;

    void onRowKeys(size_t                              row,
                   const sponge::input::InputSnapshot& input) override;

    bool onConfirm(size_t row) override;

    void onActionHint() override;

    bool onRowPressed(size_t row, const glm::vec2& mouse) override;
};

}  // namespace game::layer
