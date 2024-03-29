#include "platform/opengl/openglvertexarray.hpp"
#include "platform/opengl/gl.hpp"

namespace sponge::renderer {

OpenGLVertexArray::OpenGLVertexArray() {
    glGenVertexArrays(1, &id);
}

OpenGLVertexArray::~OpenGLVertexArray() {
    glDeleteVertexArrays(1, &id);
}

void OpenGLVertexArray::bind() const {
    glBindVertexArray(id);
}

}  // namespace sponge::renderer
