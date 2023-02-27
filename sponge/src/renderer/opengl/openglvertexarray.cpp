#include "renderer/opengl/openglvertexarray.h"

namespace Sponge {

OpenGLVertexArray::OpenGLVertexArray() {
    glGenVertexArrays(1, &id);
}

OpenGLVertexArray::~OpenGLVertexArray() {
    glDeleteVertexArrays(1, &id);
}

void OpenGLVertexArray::bind() const {
    glBindVertexArray(id);
}

}  // namespace Sponge
