#pragma once

#include "graphics/renderer/vertexarray.h"

namespace sponge::graphics::renderer {

class OpenGLVertexArray : public VertexArray {
   public:
    OpenGLVertexArray();
    ~OpenGLVertexArray() override;

    void bind() const override;

   private:
    uint32_t id = 0;
};

}  // namespace sponge::graphics::renderer
