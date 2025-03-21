#include "indexbuffer.hpp"
#include "platform/opengl/renderer/gl.hpp"

namespace sponge::platform::opengl::renderer {

IndexBuffer::IndexBuffer(const uint32_t* indices, const std::size_t size) {
    glGenBuffers(1, &id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_DYNAMIC_DRAW);
}

IndexBuffer::~IndexBuffer() {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &id);
}

void IndexBuffer::update(const uint32_t* indices,
                         const std::size_t size) const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, size * sizeof(uint32_t),
                    indices);
}

void IndexBuffer::bind() const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
}

void IndexBuffer::unbind() const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

}  // namespace sponge::platform::opengl::renderer
