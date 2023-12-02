#pragma once

#include "openglbuffer.h"
#include "openglelementbuffer.h"
#include "opengltexture.h"
#include "openglvertexarray.h"
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace sponge::graphics::renderer {

class OpenGLQuad {
   public:
    OpenGLQuad();

    void render(const glm::vec2& top, const glm::vec2& bottom,
                const glm::vec4& color);

   private:
    std::unique_ptr<OpenGLBuffer> vbo;
    std::unique_ptr<OpenGLVertexArray> vao;
    std::unique_ptr<OpenGLElementBuffer> ebo;

    static constexpr uint32_t numIndices = 6;
    static constexpr uint32_t numVertices = 8;
};

}  // namespace sponge::graphics::renderer
