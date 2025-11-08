#pragma once

#include "core/base.hpp"
#include "core/file.hpp"

#include <cstdint>
#include <string>

namespace sponge::platform::opengl::renderer {

enum LoadFlag : uint8_t {
    None                = 0,
    ExcludeAssetsFolder = BIT(0),
    GammaCorrection     = BIT(1),
    DepthMap            = BIT(2)
};

struct TextureCreateInfo {
    std::string    name;
    std::string    path;
    uint32_t       width         = 0;
    uint32_t       height        = 0;
    uint32_t       bytesPerPixel = 4;
    const uint8_t* data          = nullptr;
    const LoadFlag loadFlag      = None;
    std::string    assetsFolder  = core::File::getResourceDir();
};

class Texture final {
public:
    explicit Texture(const TextureCreateInfo& createInfo);
    ~Texture();

    uint32_t getWidth() const {
        return width;
    }
    uint32_t getHeight() const {
        return height;
    }
    uint32_t getId() const {
        return id;
    }

    void activateAndBind(uint8_t unit) const;
    void bind() const;

private:
    uint32_t id     = 0;
    uint32_t width  = 0;
    uint32_t height = 0;

    void loadFromFile(const std::string& path, uint8_t flag);
    void createDepthMap(uint32_t width, uint32_t height) const;
    void generate(uint32_t textureWidth, uint32_t textureHeight,
                  uint32_t bytesPerPixel, const uint8_t* data, uint8_t flag);
};

}  // namespace sponge::platform::opengl::renderer
