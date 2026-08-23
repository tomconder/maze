#include "platform/opengl/renderer/texture.hpp"

#include "logging/log.hpp"
#include "platform/opengl/renderer/gl.hpp"

#include <stb_image.h>

#include <filesystem>
#include <string>

namespace sponge::platform::opengl::renderer {
Texture::Texture(const TextureCreateInfo& createInfo) {
    glGenTextures(1, &id);

    if (!createInfo.path.empty()) {
        SPONGE_GL_INFO("Loading texture file: [{}, {}]", createInfo.name,
                       createInfo.path);

        const bool excludeAssetsFolder =
            (createInfo.loadFlag & ExcludeAssetsFolder) == ExcludeAssetsFolder;
        const std::string texturePath =
            excludeAssetsFolder ?
                createInfo.path :
                (std::filesystem::path(createInfo.assetsFolder) /
                 createInfo.path)
                    .string();

        loadFromFile(texturePath, createInfo.loadFlag);
    } else if (createInfo.data != nullptr) {
        SPONGE_GL_INFO("Creating texture from memory: [{}, {}x{}]",
                       createInfo.name, createInfo.width, createInfo.height);
        generate(createInfo.width, createInfo.height, createInfo.bytesPerPixel,
                 createInfo.data, createInfo.loadFlag);
    } else {
        SPONGE_GL_ERROR("Unable to create texture");
    }
}

Texture::~Texture() {
    glDeleteTextures(1, &id);
}

void Texture::generate(const uint32_t textureWidth,
                       const uint32_t textureHeight,
                       const uint32_t bytesPerPixel, const uint8_t* data,
                       const uint8_t flag) {
    width  = textureWidth;
    height = textureHeight;

    const auto gammaCorrection = (flag & GammaCorrection) == GammaCorrection;

    const auto pixelated = (flag & Pixelated) == Pixelated;

    // Sized internal formats: the unsized names let the driver pick a
    // narrower layout, which shows up as fringing on the subpixel glyph
    // atlas.
    uint32_t internalFormat = GL_RGB8;
    uint32_t format         = GL_RGB;
    if (bytesPerPixel == 1) {
        internalFormat = GL_R8;
        format         = GL_RED;
    } else if (bytesPerPixel == 3) {
        internalFormat = gammaCorrection ? GL_SRGB8 : GL_RGB8;
        format         = GL_RGB;
    } else if (bytesPerPixel == 4) {
        internalFormat = gammaCorrection ? GL_SRGB8_ALPHA8 : GL_RGBA8;
        format         = GL_RGBA;
    }

    glBindTexture(GL_TEXTURE_2D, id);
    // Rows are tightly packed. The default alignment of 4 skews any upload
    // whose width times bytesPerPixel is not a multiple of it.
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format,
                 GL_UNSIGNED_BYTE, data);

    if (pixelated) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    } else {
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
}

void Texture::loadFromFile(const std::string& path, const uint8_t flag) {
    assert(!path.empty());

    const std::filesystem::path name{ path };

    int bytesPerPixel = 0;
    int loadedHeight  = 0;
    int loadedWidth   = 0;

    auto* data = stbi_load(name.string().data(), &loadedWidth, &loadedHeight,
                           &bytesPerPixel, 0);
    if (data == nullptr) {
        SPONGE_GL_ERROR("Unable to load texture, path = {}: {}", name.string(),
                        stbi_failure_reason());
        return;
    }

    generate(loadedWidth, loadedHeight, bytesPerPixel, data, flag);

    stbi_image_free(data);
}

void Texture::activateAndBind(const uint8_t unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id);
}

void Texture::bind() const {
    glBindTexture(GL_TEXTURE_2D, id);
}

}  // namespace sponge::platform::opengl::renderer
