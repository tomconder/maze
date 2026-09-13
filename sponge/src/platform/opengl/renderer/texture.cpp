#include "platform/opengl/renderer/texture.hpp"

#include "logging/log.hpp"
#include "platform/opengl/renderer/gl.hpp"
#include "scene/ktx2.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <vector>

namespace {
// KTX2 vkFormat to the GL internal format and, for uncompressed formats, the
// upload format. compressed is false when the level data is plain pixels.
struct GlFormat {
    uint32_t internalFormat{ 0 };
    uint32_t format{ 0 };
    bool     compressed{ false };
};

GlFormat glFormatOf(const sponge::scene::ktx2::Format format) {
    namespace ktx2 = sponge::scene::ktx2;
    switch (format) {
        case ktx2::formatR8Unorm:
            return { GL_R8, GL_RED, false };
        case ktx2::formatR8G8Unorm:
            return { GL_RG8, GL_RG, false };
        case ktx2::formatR8G8B8Unorm:
            return { GL_RGB8, GL_RGB, false };
        case ktx2::formatR8G8B8A8Unorm:
            return { GL_RGBA8, GL_RGBA, false };
        case ktx2::formatR8G8B8A8Srgb:
            return { GL_SRGB8_ALPHA8, GL_RGBA, false };
        case ktx2::formatBc5Unorm:
            return { GL_COMPRESSED_RG_RGTC2, 0, true };
        case ktx2::formatBc7Unorm:
            return { GL_COMPRESSED_RGBA_BPTC_UNORM, 0, true };
        case ktx2::formatBc7Srgb:
            return { GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM, 0, true };
        default:
            return {};
    }
}
}  // namespace

namespace sponge::platform::opengl::renderer {
Texture::Texture(const TextureCreateInfo& createInfo) {
    glGenTextures(1, &id);

    if (!createInfo.ktx2.empty()) {
        SPONGE_GL_INFO("Loading baked texture: [{}]", createInfo.name);
        loadFromKtx2(createInfo.ktx2, createInfo.loadFlag);
    } else if (!createInfo.path.empty()) {
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
    if (name.extension() != ".ktx2") {
        SPONGE_GL_ERROR("Not a baked texture, path = {}", name.string());
        return;
    }

    std::ifstream file{ name, std::ios::binary | std::ios::ate };
    if (!file) {
        SPONGE_GL_ERROR("Unable to open texture, path = {}", name.string());
        return;
    }

    const auto size = static_cast<size_t>(file.tellg());
    file.seekg(0);
    std::vector<uint8_t> bytes(size);
    if (!file.read(reinterpret_cast<char*>(bytes.data()),
                   static_cast<std::streamsize>(size))) {
        SPONGE_GL_ERROR("Unable to read texture, path = {}", name.string());
        return;
    }

    loadFromKtx2(bytes, flag);
}

void Texture::loadFromKtx2(const std::span<const uint8_t> bytes,
                           const uint8_t                  flag) {
    const auto image = sponge::scene::ktx2::read(bytes);
    if (image.levels.empty()) {
        return;
    }

    const auto glFormat = glFormatOf(image.format);
    if (glFormat.internalFormat == 0) {
        SPONGE_GL_ERROR("Unsupported KTX2 format {}",
                        static_cast<uint32_t>(image.format));
        return;
    }

    width  = image.width;
    height = image.height;

    glBindTexture(GL_TEXTURE_2D, id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (uint32_t level = 0; level < image.levels.size(); level++) {
        const auto& source = image.levels[level];
        if (glFormat.compressed) {
            glCompressedTexImage2D(
                GL_TEXTURE_2D, static_cast<int32_t>(level),
                glFormat.internalFormat, static_cast<int32_t>(source.width),
                static_cast<int32_t>(source.height), 0,
                static_cast<int32_t>(source.bytes.size()), source.bytes.data());
        } else {
            glTexImage2D(GL_TEXTURE_2D, static_cast<int32_t>(level),
                         static_cast<int32_t>(glFormat.internalFormat),
                         static_cast<int32_t>(source.width),
                         static_cast<int32_t>(source.height), 0,
                         glFormat.format, GL_UNSIGNED_BYTE,
                         source.bytes.data());
        }
    }

    const auto pixelated = (flag & Pixelated) == Pixelated;
    if (pixelated) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    } else {
        // The file decides the mip chain; the driver is never asked to
        // invent one. A sprite atlas ships a single level on purpose,
        // because generated mips blend neighbouring sprites together.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL,
                        static_cast<int32_t>(image.levels.size()) - 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        image.levels.size() == 1 ? GL_LINEAR :
                                                   GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
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
