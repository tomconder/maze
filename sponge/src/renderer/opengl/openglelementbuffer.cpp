#include "openglelementbuffer.h"

OpenGLElementBuffer::OpenGLElementBuffer(uint32_t size) {
    glGenBuffers(1, &id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, nullptr, GL_STATIC_DRAW);
}

OpenGLElementBuffer::OpenGLElementBuffer(const unsigned int *indices, uint32_t size) {
    glGenBuffers(1, &id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW);
}

OpenGLElementBuffer::~OpenGLElementBuffer() {
    glDeleteBuffers(1, &id);
}

void OpenGLElementBuffer::bind() const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
}

void OpenGLElementBuffer::setData(const unsigned int *data, uint32_t size) const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, size, data);
}