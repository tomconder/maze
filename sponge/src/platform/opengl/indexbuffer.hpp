#pragma once

#include "renderer/buffer.hpp"
#include <memory>
#include <vector>

namespace sponge::platform::opengl {

class IndexBuffer final : public renderer::Buffer {
   public:
    static std::unique_ptr<IndexBuffer> create(
        const std::vector<uint32_t>& indices);
    static std::unique_ptr<IndexBuffer> create(uint32_t size);

    IndexBuffer(const IndexBuffer& indexBuffer) = delete;
    IndexBuffer& operator=(const IndexBuffer& indexBuffer) = delete;
    ~IndexBuffer() override;

    void update(const std::vector<uint32_t>& indices) const;

    void bind() const override;
    void unbind() const override;

   private:
    explicit IndexBuffer(const std::vector<uint32_t>& indices);
    explicit IndexBuffer(uint32_t size);
};

}  // namespace sponge::platform::opengl
