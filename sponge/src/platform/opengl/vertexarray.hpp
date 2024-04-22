#pragma once

#include "renderer/buffer.hpp"

namespace sponge::platform::opengl {

class VertexArray : public renderer::Buffer {
   public:
    VertexArray();
    VertexArray(const VertexArray& vertexArray);
    VertexArray(VertexArray&& vertexArray) noexcept;
    VertexArray& operator=(const VertexArray& vertexArray);
    ~VertexArray() override;

   protected:
    void init() override;

   public:
    void bind() const override;
    void unbind() const override;
};

}  // namespace sponge::platform::opengl
