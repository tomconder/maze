#include "renderer/opengl/openglbuffer.h"

namespace sponge {

OpenGLBuffer::OpenGLBuffer(uint32_t size) {
    glGenBuffers(1, &id);
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
}

OpenGLBuffer::OpenGLBuffer(const float* vertices, uint32_t size) {
    glGenBuffers(1, &id);
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_DYNAMIC_DRAW);
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

}  // namespace sponge
