#pragma once

#include "layer/audiorows.hpp"
#include "layer/scrollmenulayer.hpp"
#include "ui/slider.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace game::layer {

class AudioLayer final : public ScrollMenuLayer {
public:
    AudioLayer();

    void onAttach() override;

private:
    // One slider per row, in AudioMenuItem order.
    std::array<std::unique_ptr<ui::Slider>,
               static_cast<size_t>(AudioMenuItem::Count)>
        sliders;

    // Pushes a slider's current value to the audio engine and to Settings.
    // save chooses whether the write reaches disk now (a discrete step) or
    // waits for the drag to end (continuous mouse movement).
    void applyVolume(AudioMenuItem item, bool save) const;

protected:
    void renderRow(size_t row, float y) override;

    void onRowKeys(size_t                              row,
                   const sponge::input::InputSnapshot& input) override;

    bool onRowPressed(size_t row, const glm::vec2& mouse) override;

    void onRowDrag(size_t row, const glm::vec2& mouse) override;

    void onRowReleased(size_t row) override;

    void onFontSizeChanged() override;

    void layoutRows(const ui::MenuLayout& menuLayout,
                    const ui::RowText&    text) override;
};

}  // namespace game::layer
