#include "platform/opengl/renderer/indexbuffer.hpp"

#include "platform/opengl/renderer/gl.hpp"

namespace sponge::platform::opengl::renderer {

IndexBuffer::IndexBuffer(const uint32_t* indices, const std::size_t size) {
    glCreateBuffers(1, &id);
    glNamedBufferStorage(id, static_cast<GLsizeiptr>(size), indices, 0);
}

IndexBuffer::IndexBuffer(IndexBuffer&& other) noexcept {
    id       = other.id;
    other.id = 0;
}

IndexBuffer& IndexBuffer::operator=(IndexBuffer&& other) noexcept {
    if (this != &other) {
        if (id != 0) {
            glDeleteBuffers(1, &id);
        }
        id       = other.id;
        other.id = 0;
    }
    return *this;
}

IndexBuffer::~IndexBuffer() {
    glDeleteBuffers(1, &id);
}

}  // namespace sponge::platform::opengl::renderer
