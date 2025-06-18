#pragma once

#include "platform/opengl/renderer/buffer.hpp"

namespace sponge::platform::opengl::renderer {

class FrameBuffer final : public Buffer {
   public:
    FrameBuffer();

    FrameBuffer(const FrameBuffer& vertexBuffer) = delete;
    FrameBuffer& operator=(const FrameBuffer& vertexBuffer) = delete;
    ~FrameBuffer() override;

    void bind() const override;
    void unbind() const override;
    static bool checkStatus();
};

}  // namespace sponge::platform::opengl::renderer
