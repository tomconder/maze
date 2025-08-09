#pragma once

#include "platform/opengl/renderer/buffer.hpp"
#include <cstddef>

namespace sponge::platform::opengl::renderer {

class VertexBuffer final : Buffer {
public:
    VertexBuffer(const void* vertices, std::size_t size);

    VertexBuffer(const VertexBuffer& vertexBuffer) = delete;
    VertexBuffer& operator=(const VertexBuffer& vertexBuffer) = delete;
    ~VertexBuffer() override;

    void update(const void* vertices, std::size_t size) const;

    void bind() const override;
    void unbind() const override;
};

}  // namespace sponge::platform::opengl::renderer
