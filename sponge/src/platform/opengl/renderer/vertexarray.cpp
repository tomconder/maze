#include "platform/opengl/renderer/vertexarray.hpp"

#include "platform/opengl/renderer/gl.hpp"

#include <memory>

namespace sponge::platform::opengl::renderer {

VertexArray::VertexArray() {
    glGenVertexArrays(1, &id);
    glBindVertexArray(id);
}

VertexArray::VertexArray(VertexArray&& other) noexcept {
    id       = other.id;
    other.id = 0;
}

VertexArray& VertexArray::operator=(VertexArray&& other) noexcept {
    if (this != &other) {
        if (id != 0) {
            glDeleteVertexArrays(1, &id);
        }
        id = other.id;
    }
    return *this;
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
