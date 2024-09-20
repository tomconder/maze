#pragma once

#include "indexbuffer.hpp"
#include "renderer/sprite.hpp"
#include "texture.hpp"
#include "vertexarray.hpp"
#include "vertexbuffer.hpp"

namespace sponge::platform::opengl::renderer {

class Sprite final : public sponge::renderer::Sprite {
   public:
    explicit Sprite(const std::string& name);

    void render(glm::vec2 position, glm::vec2 size) const override;

   private:
    std::string name;
    std::unique_ptr<VertexBuffer> vbo;
    std::unique_ptr<IndexBuffer> ebo;
    std::unique_ptr<VertexArray> vao;
    std::shared_ptr<Texture> tex;
};

}  // namespace sponge::platform::opengl::renderer
