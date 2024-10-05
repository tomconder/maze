#pragma once

#include "renderer/buffer.hpp"

namespace sponge::platform::opengl::renderer {

class FrameBuffer final : public sponge::renderer::Buffer {
   public:
    FrameBuffer();
    ~FrameBuffer() override;

    void bind() const override;
    void unbind() const override;
};

}  // namespace sponge::platform::opengl::renderer
