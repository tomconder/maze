#include "openglelementbuffer.h"

namespace sponge::graphics::renderer {

OpenGLElementBuffer::OpenGLElementBuffer(uint32_t size) {
    glGenBuffers(1, &id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
}

OpenGLElementBuffer::OpenGLElementBuffer(const uint32_t* indices,
                                         uint32_t size) {
    glGenBuffers(1, &id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_DYNAMIC_DRAW);
}

OpenGLElementBuffer::~OpenGLElementBuffer() {
    glDeleteBuffers(1, &id);
}

void OpenGLElementBuffer::bind() const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
}

void OpenGLElementBuffer::setData(const uint32_t* data, uint32_t size) const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, size, data);
}

}  // namespace sponge::graphics::renderer
