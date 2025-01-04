#include "color.hpp"
#include "logging/log.hpp"
#include <algorithm>

namespace sponge::core {

glm::vec3 Color::hexToRGB(const char* hex) {
    if (hex[6] != '\0') {
        SPONGE_CORE_ERROR("Hex color must be 6 characters long.");
        return { 0.F, 0.F, 0.F };
    }

    return { hexToFloat(hex), hexToFloat(hex + 2), hexToFloat(hex + 4) };
}

void Color::rgbToHex(const glm::vec3& rgb, char* hex) {
    for (int i = 0; i < 3; i++) {
        int value =
            static_cast<int>(std::clamp(rgb[i] * 255.F + .5F, 0.F, 255.F));

        hex[i * 2] = (value >> 4) + (value > 0x9F ? 'A' - 10 : '0');
        hex[i * 2 + 1] = (value & 0xF) + ((value & 0xF) > 9 ? 'A' - 10 : '0');
    }
    hex[6] = '\0';
}

float Color::hexToFloat(const char* hex) {
    return ((hex[0] >= 'A' ? hex[0] - 'A' + 10 : hex[0] - '0') * 16 +
            (hex[1] >= 'A' ? hex[1] - 'A' + 10 : hex[1] - '0')) /
           255.F;
}

}  // namespace sponge::core
