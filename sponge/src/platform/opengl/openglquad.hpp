#pragma once

#include "platform/opengl/openglindexbuffer.hpp"
#include "platform/opengl/openglvertexarray.hpp"
#include "platform/opengl/openglvertexbuffer.hpp"
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <string>

namespace sponge::renderer {

class OpenGLQuad {
   public:
    explicit OpenGLQuad(const std::string& shaderName);

    void render(const glm::vec2& top, const glm::vec2& bottom,
                const glm::vec4& color) const;

   private:
    std::string shaderName;

    std::unique_ptr<OpenGLVertexBuffer> vbo;
    std::unique_ptr<OpenGLIndexBuffer> ebo;
    std::unique_ptr<OpenGLVertexArray> vao;
};

}  // namespace sponge::renderer
