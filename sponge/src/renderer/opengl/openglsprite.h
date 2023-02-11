#pragma once

#include "openglbuffer.h"
#include "openglvertexarray.h"
#include "renderer/sprite.h"

class OpenGLSprite : public Sprite {
   public:
    OpenGLSprite(int screenWidth, int screenHeight);

    void render(const std::string &name, glm::vec2 position, glm::vec2 size) const override;

    std::unique_ptr<OpenGLBuffer> vbo;
    std::unique_ptr<OpenGLVertexArray> vao;
};
