#pragma once

#include <cstdint>

#include <memory>

namespace sponge::platform::opengl::renderer {

class VertexArray final {
public:
    static std::unique_ptr<VertexArray> create();

    VertexArray(const VertexArray& vertexArray)            = delete;
    VertexArray& operator=(const VertexArray& vertexArray) = delete;

    VertexArray(VertexArray&& other) noexcept;
    VertexArray& operator=(VertexArray&& other) noexcept;

    ~VertexArray();

    void bind() const;
    void unbind() const;

private:
    VertexArray();

    mutable uint32_t id = 0;
};

}  // namespace sponge::platform::opengl::renderer
