#include "platform/opengl/openglbuffer.h"

namespace sponge::renderer {

OpenGLBuffer::OpenGLBuffer(uint32_t size) {
    glGenBuffers(1, &id);
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_STATIC_DRAW);
}

OpenGLBuffer::OpenGLBuffer(const float* vertices, uint32_t size) {
    glGenBuffers(1, &id);
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
}

OpenGLBuffer::~OpenGLBuffer() {
    glDeleteBuffers(1, &id);
}

void OpenGLBuffer::bind() const {
    glBindBuffer(GL_ARRAY_BUFFER, id);
}

void OpenGLBuffer::setData(const float* data, uint32_t size) const {
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
}

}  // namespace sponge::renderer
