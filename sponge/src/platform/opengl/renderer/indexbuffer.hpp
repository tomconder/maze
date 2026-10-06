#pragma once

#include <cstddef>
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

    // Overwrites `size` bytes at `offset`. For a blended mesh's triangle
    // order, which changes with the camera.
    void update(std::size_t offset, const uint32_t* indices,
                std::size_t size) const;

    uint32_t getId() const {
        return id;
    }

private:
    mutable uint32_t id = 0;
};

}  // namespace sponge::platform::opengl::renderer
