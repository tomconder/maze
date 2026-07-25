#pragma once

#include <cstdint>

#include <cstddef>

namespace sponge::platform::opengl::renderer {

class VertexBuffer final {
public:
    VertexBuffer(const void* vertices, std::size_t size);

    VertexBuffer(const VertexBuffer& vertexBuffer)            = delete;
    VertexBuffer& operator=(const VertexBuffer& vertexBuffer) = delete;

    VertexBuffer(VertexBuffer&& other) noexcept;
    VertexBuffer& operator=(VertexBuffer&& other) noexcept;

    ~VertexBuffer();

    void update(const void* vertices, std::size_t size) const;

    void bind() const;
    void unbind() const;

private:
    mutable uint32_t id = 0;
};

}  // namespace sponge::platform::opengl::renderer
