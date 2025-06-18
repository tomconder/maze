#pragma once

#include <cstdint>
#include <string>

namespace sponge::platform::opengl::renderer {

class Texture final {
   public:
    Texture();
    ~Texture();

    void createDepthMap(uint32_t width, uint32_t height) const;

    void generate(uint32_t textureWidth, uint32_t textureHeight,
                  uint32_t bytesPerPixel, const uint8_t* data,
                  bool gammaCorrection = false);

    uint32_t getWidth() const {
        return width;
    }
    uint32_t getHeight() const {
        return height;
    }
    uint32_t getId() const {
        return id;
    }
    std::string getType() const {
        return type;
    }

    void activateAndBind(uint8_t unit) const;
    void bind() const;
    void setType(const std::string_view& typeName) {
        type = typeName;
    }

   private:
    uint32_t id = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    std::string type;
};

}  // namespace sponge::platform::opengl::renderer
