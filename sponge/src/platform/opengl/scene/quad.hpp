#pragma once

#include "platform/opengl/renderer/indexbuffer.hpp"
#include "platform/opengl/renderer/vertexarray.hpp"
#include "platform/opengl/renderer/vertexbuffer.hpp"
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace sponge::platform::opengl::scene {

class Quad {
   public:
    explicit Quad();

    void render(const glm::vec2& top, const glm::vec2& bottom,
                const glm::vec4& color) const;

   private:
    std::unique_ptr<renderer::VertexBuffer> vbo;
    std::unique_ptr<renderer::IndexBuffer> ebo;
    std::unique_ptr<renderer::VertexArray> vao;
};

}  // namespace sponge::platform::opengl::scene
