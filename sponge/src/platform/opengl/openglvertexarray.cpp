#include "platform/opengl/openglvertexarray.h"

namespace sponge::graphics::renderer {

OpenGLVertexArray::OpenGLVertexArray() {
    glGenVertexArrays(1, &id);
}

OpenGLVertexArray::~OpenGLVertexArray() {
    glDeleteVertexArrays(1, &id);
}

void OpenGLVertexArray::bind() const {
    glBindVertexArray(id);
}

}  // namespace sponge::graphics::renderer
