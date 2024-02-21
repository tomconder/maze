#pragma once

#include "renderer/sprite.h"

namespace sponge::renderer {

class OpenGLSprite : public Sprite {
   public:
    explicit OpenGLSprite(std::string_view name);

    void render(glm::vec2 position, glm::vec2 size) const override;

   private:
    std::string name;
};

}  // namespace sponge::renderer
