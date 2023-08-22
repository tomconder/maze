#pragma once

#include "renderer/opengl/openglbuffer.h"
#include "renderer/opengl/openglelementbuffer.h"
#include "renderer/opengl/openglvertexarray.h"
#include "renderer/sprite.h"

namespace sponge {

class OpenGLSprite : public Sprite {
   public:
    explicit OpenGLSprite(std::string_view name);

    void render(glm::vec2 position, glm::vec2 size) const override;

   private:
    std::unique_ptr<OpenGLBuffer> vbo;
    std::unique_ptr<OpenGLElementBuffer> ebo;
    std::unique_ptr<OpenGLVertexArray> vao;

    std::string name;

    static constexpr uint32_t numIndices = 6;
    static constexpr uint32_t numVertices = 16;
};

}  // namespace sponge
