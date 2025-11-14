#include "platform/opengl/renderer/texture.hpp"

#include "logging/log.hpp"
#include "platform/opengl/renderer/gl.hpp"

#define STB_IMAGE_IMPLEMENTATION
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
    } else if ((createInfo.loadFlag & DepthMap) == DepthMap) {
        SPONGE_GL_INFO("Creating depth map texture: [{}, {}x{}]",
                       createInfo.name, createInfo.width, createInfo.height);
        createDepthMap(createInfo.width, createInfo.height);
    } else {
        SPONGE_GL_ERROR("Unable to create texture");
    }
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
                       const uint8_t flag) {
    width  = textureWidth;
    height = textureHeight;

    const auto gammaCorrection = (flag & GammaCorrection) == GammaCorrection;

    uint32_t internalFormat = GL_RGB;
    uint32_t format         = GL_RGB;
    if (bytesPerPixel == 1) {
        internalFormat = format = GL_RED;
    } else if (bytesPerPixel == 3) {
        internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
        format         = GL_RGB;
    } else if (bytesPerPixel == 4) {
        internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
        format         = GL_RGBA;
    }

    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format,
                 GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glActiveTexture(GL_TEXTURE0);
}

void Texture::loadFromFile(const std::string& path, const uint8_t flag) {
    assert(!path.empty());

    const auto gammaCorrection = (flag & GammaCorrection) == GammaCorrection;

    const std::filesystem::path name{ path };

    int bytesPerPixel = 0;
    int height        = 0;
    int width         = 0;

    void* data =
        stbi_load(name.string().data(), &width, &height, &bytesPerPixel, 0);
    if (data == nullptr) {
        SPONGE_GL_ERROR("Unable to load texture, path = {}: {}", name.string(),
                        stbi_failure_reason());
    }

    generate(width, height, bytesPerPixel, static_cast<const uint8_t*>(data),
             flag);

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
