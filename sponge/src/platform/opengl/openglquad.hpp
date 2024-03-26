#pragma once

#include "platform/opengl/openglbuffer.hpp"
#include "platform/opengl/openglelementbuffer.hpp"
#include "platform/opengl/openglvertexarray.hpp"
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace sponge::renderer {

class OpenGLQuad {
   public:
    OpenGLQuad(const std::string& shaderName);

    void render(const glm::vec2& top, const glm::vec2& bottom,
                const glm::vec4& color) const;

   private:
    std::string shaderName;

    std::unique_ptr<OpenGLBuffer> vbo;
    std::unique_ptr<OpenGLElementBuffer> ebo;
    std::unique_ptr<OpenGLVertexArray> vao;
};

}  // namespace sponge::renderer
