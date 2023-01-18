#pragma once

#include <SDL.h>

#include "gl.h"
#include "renderer/vertexarray.h"

class OpenGLVertexArray : public VertexArray {
   public:
    OpenGLVertexArray();
    ~OpenGLVertexArray() override;

    void bind() const override;

   private:
    uint32_t id = 0;
};
