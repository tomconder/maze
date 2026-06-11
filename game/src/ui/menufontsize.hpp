#pragma once

#include <cstdint>

namespace game::ui {

inline uint32_t menuFontSizeForWidth(const uint32_t windowWidth) {
    if (windowWidth < 1280) {
        return 24;
    }
    if (windowWidth < 1920) {
        return 32;
    }
    return 48;
}

}  // namespace game::ui
