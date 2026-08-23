#pragma once

#include <cstdint>

namespace sponge::platform::opengl::renderer {

class VertexArray final {
public:
    VertexArray();

    VertexArray(const VertexArray& vertexArray)            = delete;
    VertexArray& operator=(const VertexArray& vertexArray) = delete;

    VertexArray(VertexArray&& other) noexcept;
    VertexArray& operator=(VertexArray&& other) noexcept;

    ~VertexArray();

    void bind() const;
    void unbind() const;

private:
    mutable uint32_t id = 0;
};

}  // namespace sponge::platform::opengl::renderer
