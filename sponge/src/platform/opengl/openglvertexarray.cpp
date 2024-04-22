#include "openglvertexarray.hpp"
#include "core/log.hpp"
#include "platform/opengl/gl.hpp"

namespace sponge::platform::opengl {

OpenGLVertexArray::OpenGLVertexArray() {
    glGenVertexArrays(1, &id);
    glBindVertexArray(id);
}

OpenGLVertexArray::OpenGLVertexArray(const OpenGLVertexArray& vertexArray)
    : Buffer(vertexArray) {
    id = vertexArray.id;
}

OpenGLVertexArray::OpenGLVertexArray(OpenGLVertexArray&& vertexArray) noexcept {
    id = vertexArray.id;
}

OpenGLVertexArray& OpenGLVertexArray::operator=(
    const OpenGLVertexArray& vertexArray) {
    id = vertexArray.id;
    return *this;
}

OpenGLVertexArray::~OpenGLVertexArray() {
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &id);
}

void OpenGLVertexArray::init() {
    SPONGE_CORE_WARN("Not implemented");
}

void OpenGLVertexArray::bind() const {
    glBindVertexArray(id);
}

void OpenGLVertexArray::unbind() const {
    glDeleteVertexArrays(1, &id);
}

}  // namespace sponge::platform::opengl
