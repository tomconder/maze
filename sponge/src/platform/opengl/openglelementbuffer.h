#pragma once

#include <SDL.h>

#include "gl.h"
#include "renderer/buffer.h"

class OpenGLElementBuffer : public Buffer {
   public:
    explicit OpenGLElementBuffer(uint32_t size);
    OpenGLElementBuffer(const unsigned int *indices, uint32_t size);
    ~OpenGLElementBuffer() override;

    void bind() const override;
    void setData(const unsigned int *data, uint32_t size) override;

   private:
    uint32_t id = 0;
};
