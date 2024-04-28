#include "vertexarray.hpp"
#include "platform/opengl/gl.hpp"

namespace sponge::platform::opengl {

VertexArray::VertexArray() {
    glGenVertexArrays(1, &id);
    glBindVertexArray(id);
}

VertexArray::~VertexArray() {
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &id);
}

std::unique_ptr<VertexArray> VertexArray::create() {
    std::unique_ptr<VertexArray> buffer(new VertexArray());
    return buffer;
}

void VertexArray::bind() const {
    glBindVertexArray(id);
}

void VertexArray::unbind() const {
    glDeleteVertexArrays(1, &id);
}

}  // namespace sponge::platform::opengl
