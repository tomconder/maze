#pragma once

#include "indexbuffer.hpp"
#include "renderer/sprite.hpp"
#include "vertexarray.hpp"
#include "vertexbuffer.hpp"

namespace sponge::platform::opengl {

class Sprite : public renderer::Sprite {
   public:
    explicit Sprite(std::string_view name);

    void render(glm::vec2 position, glm::vec2 size) const override;

   private:
    std::string name;
    std::unique_ptr<VertexBuffer> vbo;
    std::unique_ptr<IndexBuffer> ebo;
    std::unique_ptr<VertexArray> vao;
};

}  // namespace sponge::platform::opengl
