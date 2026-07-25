#pragma once

#include <cstdint>

namespace sponge::platform::opengl::renderer {

class FrameBuffer final {
public:
    FrameBuffer();

    FrameBuffer(const FrameBuffer& vertexBuffer)            = delete;
    FrameBuffer& operator=(const FrameBuffer& vertexBuffer) = delete;

    FrameBuffer(FrameBuffer&& other) noexcept;
    FrameBuffer& operator=(FrameBuffer&& other) noexcept;

    ~FrameBuffer();

    void        bind() const;
    void        unbind() const;
    static bool checkStatus();

private:
    mutable uint32_t id = 0;
};

}  // namespace sponge::platform::opengl::renderer
