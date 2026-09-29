#include "platform/opengl/renderer/vertexarray.hpp"

#include "platform/opengl/renderer/gl.hpp"
#include "platform/opengl/renderer/indexbuffer.hpp"
#include "platform/opengl/renderer/vertexbuffer.hpp"

namespace sponge::platform::opengl::renderer {

VertexArray::VertexArray() {
    glCreateVertexArrays(1, &id);
}

VertexArray::VertexArray(VertexArray&& other) noexcept {
    id       = other.id;
    other.id = 0;
}

VertexArray& VertexArray::operator=(VertexArray&& other) noexcept {
    if (this != &other) {
        if (id != 0) {
            glDeleteVertexArrays(1, &id);
        }
        id       = other.id;
        other.id = 0;
    }
    return *this;
}

VertexArray::~VertexArray() {
    glDeleteVertexArrays(1, &id);
}

void VertexArray::setVertexBuffer(const VertexBuffer& vertexBuffer,
                                  const uint32_t      stride) const {
    glVertexArrayVertexBuffer(id, 0, vertexBuffer.getId(), 0,
                              static_cast<GLsizei>(stride));
}

void VertexArray::setIndexBuffer(const IndexBuffer& indexBuffer) const {
    glVertexArrayElementBuffer(id, indexBuffer.getId());
}

void VertexArray::addAttribute(const uint32_t location, const int32_t count,
                               const uint32_t offset) const {
    glEnableVertexArrayAttrib(id, location);
    glVertexArrayAttribFormat(id, location, count, GL_FLOAT, GL_FALSE, offset);
    glVertexArrayAttribBinding(id, location, 0);
}

void VertexArray::bind() const {
    glBindVertexArray(id);
}

void VertexArray::unbind() const {
    glBindVertexArray(0);
}

}  // namespace sponge::platform::opengl::renderer
