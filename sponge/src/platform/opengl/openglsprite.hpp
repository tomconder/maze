#pragma once

#include "openglindexbuffer.hpp"
#include "openglvertexarray.hpp"
#include "openglvertexbuffer.hpp"
#include "renderer/sprite.hpp"

namespace sponge::renderer {

class OpenGLSprite : public Sprite {
   public:
    explicit OpenGLSprite(std::string_view name);

    void render(glm::vec2 position, glm::vec2 size) const override;

   private:
    std::string name;
    std::unique_ptr<OpenGLVertexBuffer> vbo;
    std::unique_ptr<OpenGLIndexBuffer> ebo;
    std::unique_ptr<OpenGLVertexArray> vao;
};

}  // namespace sponge::renderer
