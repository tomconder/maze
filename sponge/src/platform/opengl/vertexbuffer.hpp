#pragma once

#include "renderer/buffer.hpp"
#include "renderer/mesh.hpp"
#include <memory>
#include <vector>

namespace sponge::platform::opengl {

class VertexBuffer : public renderer::Buffer {
   public:
    static std::unique_ptr<VertexBuffer> create(uint32_t size);
    static std::unique_ptr<VertexBuffer> create(
        const std::vector<glm::vec2>& vertices);
    static std::unique_ptr<VertexBuffer> create(
        const std::vector<renderer::Vertex>& vertices);

    VertexBuffer(const VertexBuffer& vertexBuffer) = delete;
    VertexBuffer& operator=(const VertexBuffer& vertexBuffer) = delete;
    ~VertexBuffer() override;

    void update(const std::vector<glm::vec2>& vertices) const;
    void update(const std::vector<renderer::Vertex>& vertices) const;

    void bind() const override;
    void unbind() const override;

   private:
    explicit VertexBuffer(uint32_t size);
    explicit VertexBuffer(const std::vector<glm::vec2>& vertices);
    explicit VertexBuffer(const std::vector<renderer::Vertex>& vertices);
};

}  // namespace sponge::platform::opengl
