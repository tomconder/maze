#pragma once

#include "gl.h"
#include "renderer/vertexarray.h"

namespace Sponge {

class OpenGLVertexArray : public VertexArray {
   public:
    OpenGLVertexArray();
    ~OpenGLVertexArray() override;

    void bind() const override;

   private:
    uint32_t id = 0;
};

}  // namespace Sponge
