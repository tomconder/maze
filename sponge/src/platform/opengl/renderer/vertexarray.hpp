#pragma once

#include <cstdint>

namespace sponge::platform::opengl::renderer {

class IndexBuffer;
class VertexBuffer;

class VertexArray final {
public:
    VertexArray();

    VertexArray(const VertexArray& vertexArray)            = delete;
    VertexArray& operator=(const VertexArray& vertexArray) = delete;

    VertexArray(VertexArray&& other) noexcept;
    VertexArray& operator=(VertexArray&& other) noexcept;

    ~VertexArray();

    // Setup uses one vertex buffer binding point, 0, and float attributes.
    void setVertexBuffer(const VertexBuffer& vertexBuffer,
                         uint32_t            stride) const;
    void setIndexBuffer(const IndexBuffer& indexBuffer) const;
    void addAttribute(uint32_t location, int32_t count, uint32_t offset) const;

    void bind() const;
    void unbind() const;

private:
    mutable uint32_t id = 0;
};

}  // namespace sponge::platform::opengl::renderer
