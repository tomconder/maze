#pragma once

#include "renderer/buffer.hpp"
#include <vector>

namespace sponge::platform::opengl {

class OpenGLIndexBuffer : public renderer::Buffer {
   public:
    OpenGLIndexBuffer();
    explicit OpenGLIndexBuffer(const std::vector<unsigned int>& indices);
    OpenGLIndexBuffer(uint32_t size);
    OpenGLIndexBuffer(const OpenGLIndexBuffer& indexBuffer);
    OpenGLIndexBuffer(OpenGLIndexBuffer&& indexBuffer) noexcept;
    OpenGLIndexBuffer& operator=(const OpenGLIndexBuffer& indexBuffer);
    ~OpenGLIndexBuffer() override;

   protected:
    void init() override;

   public:
    void init(const std::vector<uint32_t>& indices);
    void update(const std::vector<uint32_t>& indices) const;

    void bind() const override;
    void unbind() const override;
};

}  // namespace sponge::platform::opengl
