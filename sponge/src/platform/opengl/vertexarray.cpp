#include "vertexarray.hpp"
#include "core/log.hpp"
#include "platform/opengl/gl.hpp"

namespace sponge::platform::opengl {

VertexArray::VertexArray() {
    glGenVertexArrays(1, &id);
    glBindVertexArray(id);
}

VertexArray::VertexArray(const VertexArray& vertexArray) : Buffer(vertexArray) {
    id = vertexArray.id;
}

VertexArray::VertexArray(VertexArray&& vertexArray) noexcept {
    id = vertexArray.id;
}

VertexArray& VertexArray::operator=(const VertexArray& vertexArray) {
    id = vertexArray.id;
    return *this;
}

VertexArray::~VertexArray() {
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &id);
}

void VertexArray::init() {
    SPONGE_CORE_WARN("Not implemented");
}

void VertexArray::bind() const {
    glBindVertexArray(id);
}

void VertexArray::unbind() const {
    glDeleteVertexArrays(1, &id);
}

}  // namespace sponge::platform::opengl
