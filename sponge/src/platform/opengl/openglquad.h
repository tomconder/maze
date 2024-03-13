#pragma once

#include "platform/opengl/openglbuffer.h"
#include "platform/opengl/openglelementbuffer.h"
#include "platform/opengl/openglvertexarray.h"
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace sponge::renderer {

class OpenGLQuad {
   public:
    OpenGLQuad();

    void render(const glm::vec2& top, const glm::vec2& bottom,
                const glm::vec4& color) const;

   private:
    std::unique_ptr<OpenGLBuffer> vbo;
    std::unique_ptr<OpenGLElementBuffer> ebo;
    std::unique_ptr<OpenGLVertexArray> vao;
};

}  // namespace sponge::renderer
