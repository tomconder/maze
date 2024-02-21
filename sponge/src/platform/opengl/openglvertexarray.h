#pragma once

#include "renderer/vertexarray.h"

namespace sponge::renderer {

class OpenGLVertexArray : public VertexArray {
   public:
    OpenGLVertexArray();
    ~OpenGLVertexArray() override;

    void bind() const override;

   private:
    uint32_t id = 0;
};

}  // namespace sponge::renderer
