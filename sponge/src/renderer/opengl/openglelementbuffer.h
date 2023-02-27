#pragma once

#include "renderer/buffer.h"
#include "renderer/opengl/gl.h"

namespace Sponge {

class OpenGLElementBuffer : public Buffer {
   public:
    explicit OpenGLElementBuffer(uint32_t size);
    OpenGLElementBuffer(const uint32_t *indices, uint32_t size);
    ~OpenGLElementBuffer() override;

    void bind() const override;
    void setData(const uint32_t *data, uint32_t size) const;

   private:
    uint32_t id = 0;
};

}  // namespace Sponge
