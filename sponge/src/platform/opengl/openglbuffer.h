#pragma once

#include "graphics/renderer/buffer.h"
#include "platform/opengl/gl.h"

namespace sponge::graphics::renderer {

class OpenGLBuffer : public Buffer {
   public:
    explicit OpenGLBuffer(uint32_t size);
    OpenGLBuffer(const float* vertices, uint32_t size);
    ~OpenGLBuffer() override;

    void bind() const override;
    void setData(const float* data, uint32_t size) const;

   private:
    uint32_t id = 0;
};

}  // namespace sponge::graphics::renderer