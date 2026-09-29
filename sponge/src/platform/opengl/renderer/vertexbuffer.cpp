#include "platform/opengl/renderer/vertexbuffer.hpp"

#include "platform/opengl/renderer/gl.hpp"

namespace sponge::platform::opengl::renderer {

VertexBuffer::VertexBuffer(const void* vertices, const std::size_t size) {
    glCreateBuffers(1, &id);
    glNamedBufferStorage(id, static_cast<GLsizeiptr>(size), vertices,
                         GL_DYNAMIC_STORAGE_BIT);
}

VertexBuffer::VertexBuffer(VertexBuffer&& other) noexcept {
    id       = other.id;
    other.id = 0;
}

VertexBuffer& VertexBuffer::operator=(VertexBuffer&& other) noexcept {
    if (this != &other) {
        if (id != 0) {
            glDeleteBuffers(1, &id);
        }
        id       = other.id;
        other.id = 0;
    }
    return *this;
}

VertexBuffer::~VertexBuffer() {
    glDeleteBuffers(1, &id);
}

void VertexBuffer::update(const void* vertices, const std::size_t size) const {
    glNamedBufferSubData(id, 0, static_cast<GLsizeiptr>(size), vertices);
}

}  // namespace sponge::platform::opengl::renderer
