#pragma once

#include "platform/opengl/renderer/indexbuffer.hpp"
#include "platform/opengl/renderer/vertexarray.hpp"
#include "platform/opengl/renderer/vertexbuffer.hpp"
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <string>

namespace sponge::platform::opengl::renderer {

class Quad {
   public:
    explicit Quad(const std::string& shaderName);

    void render(const glm::vec2& top, const glm::vec2& bottom,
                const glm::vec4& color) const;

   private:
    std::string shaderName;

    std::unique_ptr<VertexBuffer> vbo;
    std::unique_ptr<IndexBuffer> ebo;
    std::unique_ptr<VertexArray> vao;
};

}  // namespace sponge::platform::opengl
