#pragma once

#include <cstdint>

namespace game::ui {

inline uint32_t menuFontSizeForWidth(const uint32_t windowWidth) {
    if (windowWidth < 1024) {
        return 32;
    }
    if (windowWidth < 1440) {
        return 40;
    }
    return 48;
}

}  // namespace game::ui
