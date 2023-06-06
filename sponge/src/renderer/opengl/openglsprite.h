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

    const std::vector<uint32_t> indices = std::vector<uint32_t>{
        0, 1, 2,  //
        0, 2, 3   //
    };

    std::string name;
};

}  // namespace sponge
