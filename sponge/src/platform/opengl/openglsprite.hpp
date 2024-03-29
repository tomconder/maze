#pragma once

#include "openglbuffer.hpp"
#include "openglelementbuffer.hpp"
#include "openglvertexarray.hpp"
#include "renderer/sprite.hpp"

namespace sponge::renderer {

class OpenGLSprite : public Sprite {
   public:
    explicit OpenGLSprite(std::string_view name);

    void render(glm::vec2 position, glm::vec2 size) const override;

   private:
    std::string name;
    std::unique_ptr<OpenGLBuffer> vbo;
    std::unique_ptr<OpenGLElementBuffer> ebo;
    std::unique_ptr<OpenGLVertexArray> vao;
};

}  // namespace sponge::renderer
