#pragma once

#include "renderer/buffer.hpp"
#include "renderer/mesh.hpp"
#include <vector>

namespace sponge::platform::opengl {

class OpenGLVertexBuffer : public renderer::Buffer {
   public:
    OpenGLVertexBuffer();
    explicit OpenGLVertexBuffer(const std::vector<glm::vec2>& vertices);
    explicit OpenGLVertexBuffer(const std::vector<renderer::Vertex>& vertices);
    OpenGLVertexBuffer(uint32_t size);
    OpenGLVertexBuffer(const OpenGLVertexBuffer& vertexBuffer);
    OpenGLVertexBuffer(OpenGLVertexBuffer&& vertexBuffer) noexcept;
    OpenGLVertexBuffer& operator=(const OpenGLVertexBuffer& vertexBuffer);
    ~OpenGLVertexBuffer() override;

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
