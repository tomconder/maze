#pragma once

#include "renderer/opengl/openglbuffer.h"
#include "renderer/opengl/openglelementbuffer.h"
#include "renderer/opengl/opengltexture.h"
#include "renderer/opengl/openglvertexarray.h"

namespace sponge {

class OpenGLQuad {
   public:
    OpenGLQuad();

    void render(const glm::vec2& top, const glm::vec2& bottom,
                const glm::vec4& color);

   private:
    std::unique_ptr<OpenGLBuffer> vbo;
    std::unique_ptr<OpenGLVertexArray> vao;
    std::unique_ptr<OpenGLElementBuffer> ebo;
};

}  // namespace sponge