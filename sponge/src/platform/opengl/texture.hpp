#pragma once

#include "renderer/texture.hpp"
#include <string>

namespace sponge::platform::opengl {

class Texture final : public renderer::Texture {
   public:
    Texture();
    ~Texture() override;

    void generate(uint32_t textureWidth, uint32_t textureHeight,
                  uint32_t bytesPerPixel, const uint8_t* data);

    uint32_t getWidth() const override {
        return width;
    }
    uint32_t getHeight() const override {
        return height;
    }
    uint32_t getId() const override {
        return id;
    }
    std::string getType() const {
        return type;
    }

    void bind() const override;
    void setType(const std::string_view& typeName) {
        type = typeName;
    }

   private:
    uint32_t id = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t format = 0;
    std::string type;
};

}  // namespace sponge::platform::opengl
