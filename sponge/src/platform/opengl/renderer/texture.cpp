#include "texture.hpp"
#include "platform/opengl/renderer/gl.hpp"

namespace sponge::platform::opengl::renderer {

Texture::Texture() {
    glGenTextures(1, &id);
}

void Texture::generate(const uint32_t textureWidth,
                       const uint32_t textureHeight,
                       const uint32_t bytesPerPixel, const uint8_t* data) {
    glBindTexture(GL_TEXTURE_2D, id);

    width = textureWidth;
    height = textureHeight;

    format = GL_RGB;
    if (bytesPerPixel == 4) {
        format = GL_RGBA;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, format, textureWidth, textureHeight, 0,
                 format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glActiveTexture(GL_TEXTURE0);
}

Texture::~Texture() {
    glDeleteTextures(1, &id);
}

void Texture::bind() const {
    glBindTexture(GL_TEXTURE_2D, id);
}

}  // namespace sponge::platform::opengl
