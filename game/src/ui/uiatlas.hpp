#pragma once

#include "platform/opengl/scene/spriteatlas.hpp"

namespace game::ui {

// The baked UI sprite sheet: the splash logo and the input prompt icons.
// Loaded on first use, which needs a current GL context.
const sponge::platform::opengl::scene::SpriteAtlas& uiAtlas();

}  // namespace game::ui
