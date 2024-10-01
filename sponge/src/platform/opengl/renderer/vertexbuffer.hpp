#pragma once

#include "renderer/buffer.hpp"
#include "scene/mesh.hpp"
#include <glm/vec2.hpp>
#include <memory>
#include <vector>

namespace sponge::platform::opengl::renderer {

class VertexBuffer final : public sponge::renderer::Buffer {
   public:
    static std::unique_ptr<VertexBuffer> create(uint32_t size);
    static std::unique_ptr<VertexBuffer> create(
        const std::vector<glm::vec2>& vertices);
    static std::unique_ptr<VertexBuffer> create(
        const std::vector<scene::Vertex>& vertices);

    VertexBuffer(const VertexBuffer& vertexBuffer) = delete;
    VertexBuffer& operator=(const VertexBuffer& vertexBuffer) = delete;
    ~VertexBuffer() override;

    void update(const glm::vec2 vertices[], std::size_t size) const;
    void update(const std::vector<scene::Vertex>& vertices) const;

    void bind() const override;
    void unbind() const override;

   private:
    explicit VertexBuffer(uint32_t size);
    explicit VertexBuffer(const std::vector<glm::vec2>& vertices);
    explicit VertexBuffer(const std::vector<scene::Vertex>& vertices);
};

}  // namespace sponge::platform::opengl::renderer
