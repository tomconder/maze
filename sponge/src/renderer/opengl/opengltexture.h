#pragma once

#include <string>

#include "renderer/opengl/gl.h"
#include "renderer/texture.h"

namespace Sponge {

class OpenGLTexture : public Texture {
   public:
    OpenGLTexture();
    ~OpenGLTexture() override;

    void generate(uint32_t textureWidth, uint32_t textureHeight, uint32_t bytesPerPixel, const unsigned char *data);

    uint32_t getWidth() const override {
        return width;
    };
    uint32_t getHeight() const override {
        return height;
    };
    uint32_t getId() const override {
        return id;
    };
    std::string getType() const {
        return type;
    };

    void bind() const override;
    void setType(const std::string_view &typeName) {
        type = typeName;
    }

   private:
    GLuint id = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    GLenum format = 0;
    std::string type;
};

}  // namespace Sponge
