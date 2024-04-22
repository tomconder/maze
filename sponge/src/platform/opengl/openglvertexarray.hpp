#pragma once

#include "renderer/buffer.hpp"
#include <vector>

namespace sponge::renderer {

class OpenGLVertexArray : public Buffer {
   public:
    OpenGLVertexArray();
    explicit OpenGLVertexArray(const std::vector<unsigned int>& indices);
    OpenGLVertexArray(const OpenGLVertexArray& vertexArray);
    OpenGLVertexArray(OpenGLVertexArray&& vertexArray) noexcept;
    OpenGLVertexArray& operator=(const OpenGLVertexArray& vertexArray);
    ~OpenGLVertexArray() override;

   protected:
    void init() override;

   public:
    void init(const std::vector<uint32_t>& indices);
    void bind() const override;
    void unbind() const override;
};

}  // namespace sponge::renderer
