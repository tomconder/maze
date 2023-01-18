#pragma once

#include <memory>

#include "openglbuffer.h"
#include "openglvertexarray.h"
#include "renderer/sprite.h"

class OpenGLSprite : public Sprite {
   public:
    OpenGLSprite();

    void render(const std::string &name, glm::vec2 position, glm::vec2 size) const;

    std::unique_ptr<OpenGLBuffer> vbo;
    std::unique_ptr<OpenGLVertexArray> vao;
};
