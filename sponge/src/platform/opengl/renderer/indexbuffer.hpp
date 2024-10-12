#pragma once

#include "renderer/buffer.hpp"
#include <memory>
#include <vector>

namespace sponge::platform::opengl::renderer {

class IndexBuffer final : public sponge::renderer::Buffer {
   public:
    IndexBuffer(const uint32_t* indices, std::size_t size);

    IndexBuffer(const IndexBuffer& indexBuffer) = delete;
    IndexBuffer& operator=(const IndexBuffer& indexBuffer) = delete;
    ~IndexBuffer() override;

    void update(const uint32_t* indices, std::size_t size) const;

    void bind() const override;
    void unbind() const override;

   private:
    explicit IndexBuffer(const std::vector<uint32_t>& indices);
    explicit IndexBuffer(uint32_t size);
};

}  // namespace sponge::platform::opengl::renderer
