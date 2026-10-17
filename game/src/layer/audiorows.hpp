#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "ui/menulayout.hpp"
#include "ui/rowlayout.hpp"

namespace game::layer {

enum class AudioMenuItem : uint8_t {
    MasterVolume = 0,
    SfxVolume,
    MusicVolume,
    Count,
};

inline constexpr size_t audioRowCount =
    static_cast<size_t>(AudioMenuItem::Count);

// Rows follow AudioMenuItem order, so the enum is the row index.
inline constexpr std::array<std::string_view, audioRowCount> volumeLabels = {
    "Master Volume",
    "Sound Effects",
    "Music",
};

// Every row of the audio tab is a slider. Needs no window.
void layoutAudioRows(const ui::MenuLayout& layout, const ui::RowText& text);

}  // namespace game::layer
