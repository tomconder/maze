#include "platform/opengl/scene/spriteatlas.hpp"

#include "core/file.hpp"
#include "ktx2.hpp"
#include "logging/log.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"
#include "readbytes.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
constexpr std::string_view rectTableKey = "spongeAtlas";
}  // namespace

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;

SpriteAtlas::SpriteAtlas(const std::string& name, const std::string& path) {
    const auto fullPath =
        (std::filesystem::path(core::File::getResourceDir()) / path).string();

    const auto bytes = sponge::scene::readBytes(fullPath);
    if (bytes.empty()) {
        SPONGE_GL_ERROR("Unable to read atlas: {}", path);
        return;
    }

    std::string error;
    const auto  image = sponge::scene::ktx2::read(bytes, error);
    if (image.levels.empty()) {
        SPONGE_GL_ERROR("{}: {}", path, error);
        return;
    }

    const renderer::TextureCreateInfo textureCreateInfo{
        .name = name,
        .path = "",
        .ktx2 = bytes,
    };
    tex = AssetManager::createTexture(textureCreateInfo);

    const auto entry =
        std::ranges::find_if(image.keyValues, [](const auto& pair) {
            return pair.first == rectTableKey;
        });
    if (entry == image.keyValues.end()) {
        SPONGE_GL_ERROR("Atlas {} carries no rect table", path);
        return;
    }

    // "name x y w h" per line, written by the converter.
    std::istringstream table{ std::string(
        reinterpret_cast<const char*>(entry->second.data()),
        entry->second.size()) };
    std::string        spriteName;
    uint32_t           x = 0;
    uint32_t           y = 0;
    uint32_t           w = 0;
    uint32_t           h = 0;
    while (table >> spriteName >> x >> y >> w >> h) {
        const auto width  = static_cast<float>(image.width);
        const auto height = static_cast<float>(image.height);
        sprites.emplace(spriteName,
                        std::make_unique<Sprite>(
                            tex,
                            glm::vec2{ static_cast<float>(x) / width,
                                       static_cast<float>(y) / height },
                            glm::vec2{ static_cast<float>(w) / width,
                                       static_cast<float>(h) / height }));
    }

    SPONGE_GL_INFO("Loaded atlas {}: {} sprites in {}x{}", path, sprites.size(),
                   image.width, image.height);
}

const Sprite& SpriteAtlas::sprite(const std::string_view spriteName) const {
    if (const auto found = sprites.find(std::string(spriteName));
        found != sprites.end()) {
        return *found->second;
    }

    if (!missing) {
        // Logged on the first miss only: this runs per sprite per frame.
        SPONGE_GL_ERROR("Atlas has no sprite named {}", spriteName);

        // Zero UV scale: it samples one texel and covers no area, so a
        // missing sprite is invisible rather than fatal.
        missing = std::make_unique<Sprite>(tex, glm::vec2{ 0.F, 0.F },
                                           glm::vec2{ 0.F, 0.F });
    }
    return *missing;
}

}  // namespace sponge::platform::opengl::scene
