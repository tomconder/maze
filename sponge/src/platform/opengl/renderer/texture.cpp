#include "texture.hpp"
#include "platform/opengl/renderer/gl.hpp"

namespace sponge::platform::opengl::renderer {

Texture::Texture() {
    glGenTextures(1, &id);
}

Texture::~Texture() {
    glDeleteTextures(1, &id);
}

void Texture::createDepthMap(const uint32_t width,
                             const uint32_t height) const {
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    constexpr float borderColor[] = { 1.F, 1.F, 1.F, 1.F };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
}

void Texture::generate(const uint32_t textureWidth,
                       const uint32_t textureHeight,
                       const uint32_t bytesPerPixel, const uint8_t* data,
                       const bool gammaCorrection) {
    width = textureWidth;
    height = textureHeight;

    uint32_t internalFormat = GL_RGB;
    uint32_t format = GL_RGB;
    if (bytesPerPixel == 1) {
        internalFormat = format = GL_RED;
    } else if (bytesPerPixel == 3) {
        internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
        format = GL_RGB;
    } else if (bytesPerPixel == 4) {
        internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
        format = GL_RGBA;
    }

    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format,
                 GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glActiveTexture(GL_TEXTURE0);
}

void Texture::activateAndBind(const uint8_t unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id);
}

void Texture::bind() const {
    glBindTexture(GL_TEXTURE_2D, id);
}

}  // namespace sponge::platform::opengl::renderer
