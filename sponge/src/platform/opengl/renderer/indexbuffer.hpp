#pragma once

#include <cstdint>

#include <vector>

namespace sponge::platform::opengl::renderer {

class IndexBuffer final {
public:
    IndexBuffer(const uint32_t* indices, std::size_t size);

    IndexBuffer(const IndexBuffer& indexBuffer)            = delete;
    IndexBuffer& operator=(const IndexBuffer& indexBuffer) = delete;

    IndexBuffer(IndexBuffer&& other) noexcept;
    IndexBuffer& operator=(IndexBuffer&& other) noexcept;

    ~IndexBuffer();

    void update(const uint32_t* indices, std::size_t size) const;

    void bind() const;
    void unbind() const;

private:
    mutable uint32_t id = 0;
};

}  // namespace sponge::platform::opengl::renderer
