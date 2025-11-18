#include "platform/opengl/renderer/vertexarray.hpp"

#include "platform/opengl/renderer/gl.hpp"

#include <memory>

namespace sponge::platform::opengl::renderer {

VertexArray::VertexArray() {
    glGenVertexArrays(1, &id);
    glBindVertexArray(id);
}

VertexArray::~VertexArray() {
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &id);
}

std::unique_ptr<VertexArray> VertexArray::create() {
    return std::unique_ptr<VertexArray>(new VertexArray());
}

void VertexArray::bind() const {
    glBindVertexArray(id);
}

void VertexArray::unbind() const {
    glBindVertexArray(0);
}

}  // namespace sponge::platform::opengl::renderer
