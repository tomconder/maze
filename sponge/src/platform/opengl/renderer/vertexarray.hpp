#pragma once

#include "platform/opengl/renderer/buffer.hpp"
#include <memory>

namespace sponge::platform::opengl::renderer {

class VertexArray final : public Buffer {
   public:
    static std::unique_ptr<VertexArray> create();

    VertexArray(const VertexArray& vertexArray) = delete;
    VertexArray& operator=(const VertexArray& vertexArray) = delete;
    ~VertexArray() override;

    void bind() const override;
    void unbind() const override;

   private:
    VertexArray();
};

}  // namespace sponge::platform::opengl::renderer
