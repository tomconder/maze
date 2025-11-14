#pragma once

#include <glm/glm.hpp>
#include <string>
#include <string_view>

namespace sponge::core {

class Color {
public:
    static glm::vec3 hexToRGB(std::string_view hex);
    static void      rgbToHex(const glm::vec3& rgb, char* hex);

private:
    inline static float hexToFloat(std::string_view hex);
};

}  // namespace sponge::core
