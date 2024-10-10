#pragma once

#include "renderer/buffer.hpp"

namespace sponge::platform::opengl::renderer {

class FrameBuffer final : public sponge::renderer::Buffer {
   public:
    FrameBuffer();

    FrameBuffer(const FrameBuffer& vertexBuffer) = delete;
    FrameBuffer& operator=(const FrameBuffer& vertexBuffer) = delete;
    ~FrameBuffer() override;

    void bind() const override;
    void unbind() const override;
};

}  // namespace sponge::platform::opengl::renderer
