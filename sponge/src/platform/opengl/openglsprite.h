#pragma once

#include "graphics/renderer/sprite.h"
#include "platform/opengl/openglbuffer.h"
#include "platform/opengl/openglelementbuffer.h"
#include "platform/opengl/openglvertexarray.h"

namespace sponge::graphics::renderer {

class OpenGLSprite : public Sprite {
   public:
    explicit OpenGLSprite(std::string_view name);

    void render(glm::vec2 position, glm::vec2 size) const override;

   private:
    std::string name;
};

}  // namespace sponge::graphics::renderer
