#include "ui/uiatlas.hpp"

#include "platform/opengl/scene/spriteatlas.hpp"

namespace game::ui {

const sponge::platform::opengl::scene::SpriteAtlas& uiAtlas() {
    static const sponge::platform::opengl::scene::SpriteAtlas atlas{
        "ui", "textures/ui.ktx2"
    };
    return atlas;
}

}  // namespace game::ui
