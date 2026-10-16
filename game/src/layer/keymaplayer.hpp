#pragma once

#include "layer/scrollmenulayer.hpp"

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

class KeyMapLayer final : public ScrollMenuLayer {
public:
    KeyMapLayer();

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

    void onRowKeys(size_t                              row,
                   const sponge::input::InputSnapshot& input) override;

    bool onConfirm(size_t row) override;

    void onActionHint() override;

    bool onRowPressed(size_t row, const glm::vec2& mouse) override;
};

}  // namespace game::layer
