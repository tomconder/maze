#pragma once

#include "renderer/buffer.hpp"
#include "renderer/mesh.hpp"
#include <vector>

namespace sponge::platform::opengl {

class VertexBuffer : public renderer::Buffer {
   public:
    VertexBuffer();
    explicit VertexBuffer(const std::vector<glm::vec2>& vertices);
    explicit VertexBuffer(const std::vector<renderer::Vertex>& vertices);
    VertexBuffer(uint32_t size);
    VertexBuffer(const VertexBuffer& vertexBuffer);
    VertexBuffer(VertexBuffer&& vertexBuffer) noexcept;
    VertexBuffer& operator=(const VertexBuffer& vertexBuffer);
    ~VertexBuffer() override;

   protected:
    void init() override;
    void init(const float* data, uint32_t size) const;

   public:
    void update(const std::vector<glm::vec2>& vertices) const;
    void update(const std::vector<renderer::Vertex>& vertices) const;

    void bind() const override;
    void unbind() const override;
};

}  // namespace sponge::platform::opengl
