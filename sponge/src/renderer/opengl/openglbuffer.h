#pragma once

#include "renderer/buffer.h"
#include "renderer/opengl/gl.h"

namespace Sponge {

class OpenGLBuffer : public Buffer {
   public:
    explicit OpenGLBuffer(uint32_t size);
    OpenGLBuffer(const float *vertices, uint32_t size);
    ~OpenGLBuffer() override;

    void bind() const override;
    void setData(const float *data, uint32_t size) const;

   private:
    uint32_t id = 0;
};

}  // namespace Sponge
