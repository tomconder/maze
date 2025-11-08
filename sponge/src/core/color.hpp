#pragma once

#include <glm/glm.hpp>

namespace sponge::core {

class Color {
public:
    static glm::vec3 hexToRGB(const char* hex);
    static void      rgbToHex(const glm::vec3& rgb, char* hex);

private:
    inline static float hexToFloat(const char* hex);
};

}  // namespace sponge::core
