#pragma once

#include "renderer/buffer.hpp"
#include <vector>

namespace sponge::platform::opengl {

class IndexBuffer : public renderer::Buffer {
   public:
    IndexBuffer();
    explicit IndexBuffer(const std::vector<unsigned int>& indices);
    IndexBuffer(uint32_t size);
    IndexBuffer(const IndexBuffer& indexBuffer);
    IndexBuffer(IndexBuffer&& indexBuffer) noexcept;
    IndexBuffer& operator=(const IndexBuffer& indexBuffer);
    ~IndexBuffer() override;

   protected:
    void init() override;

   public:
    void init(const std::vector<uint32_t>& indices);
    void update(const std::vector<uint32_t>& indices) const;

    void bind() const override;
    void unbind() const override;
};

}  // namespace sponge::platform::opengl
