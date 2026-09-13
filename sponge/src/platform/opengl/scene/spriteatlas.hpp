#pragma once

#include "platform/opengl/renderer/texture.hpp"
#include "platform/opengl/scene/sprite.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace sponge::platform::opengl::scene {

// A baked KTX2 sprite sheet plus the rect table from its key/value data.
// Hands out Sprites that share the one texture. GL thread only.
class SpriteAtlas final {
public:
    // path is relative to the assets folder.
    explicit SpriteAtlas(const std::string& name, const std::string& path);

    // Never fails: a name the atlas does not hold logs once and returns a
    // sprite that draws nothing, so a missing icon cannot take the frame
    // down.
    const Sprite& sprite(std::string_view spriteName) const;

private:
    // Returns nullptr when the atlas has no sprite of that name.
    const Sprite* find(std::string_view spriteName) const;

    std::shared_ptr<renderer::Texture>                       tex;
    std::unordered_map<std::string, std::unique_ptr<Sprite>> sprites;
    mutable std::unique_ptr<Sprite>                          missing;
};

}  // namespace sponge::platform::opengl::scene
