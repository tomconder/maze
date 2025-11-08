#pragma once

#include "platform/opengl/renderer/buffer.hpp"

#include <vector>

namespace sponge::platform::opengl::renderer {

class IndexBuffer final : public Buffer {
public:
    IndexBuffer(const uint32_t* indices, std::size_t size);

    IndexBuffer(const IndexBuffer& indexBuffer)            = delete;
    IndexBuffer& operator=(const IndexBuffer& indexBuffer) = delete;
    ~IndexBuffer() override;

    void update(const uint32_t* indices, std::size_t size) const;

    void bind() const override;
    void unbind() const override;
};

}  // namespace sponge::platform::opengl::renderer
