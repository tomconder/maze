#pragma once

#include <glm/glm.hpp>
#include <string>

namespace sponge::core {

class Color {
public:
    static glm::vec3 hexToRGB(const std::string& hex);
    static void      rgbToHex(const glm::vec3& rgb, char* hex);

private:
    inline static float hexToFloat(const std::string& hex);
};

}  // namespace sponge::core
