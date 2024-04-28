#include "indexbuffer.hpp"
#include "platform/opengl/gl.hpp"

namespace sponge::platform::opengl {

IndexBuffer::IndexBuffer(const std::vector<uint32_t>& indices) {
    glGenBuffers(1, &id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t),
                 indices.data(), GL_DYNAMIC_DRAW);
}

IndexBuffer::IndexBuffer(const uint32_t size) {
    glGenBuffers(1, &id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size * sizeof(uint32_t), nullptr,
                 GL_DYNAMIC_DRAW);
}

IndexBuffer::~IndexBuffer() {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &id);
}

std::unique_ptr<IndexBuffer> IndexBuffer::create(
    const std::vector<uint32_t>& indices) {
    std::unique_ptr<IndexBuffer> buffer(new IndexBuffer(indices));
    return buffer;
}

std::unique_ptr<IndexBuffer> IndexBuffer::create(const uint32_t size) {
    std::unique_ptr<IndexBuffer> buffer(new IndexBuffer(size));
    return buffer;
}

void IndexBuffer::update(const std::vector<uint32_t>& indices) const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0,
                    indices.size() * sizeof(uint32_t), indices.data());
}

void IndexBuffer::bind() const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
}

void IndexBuffer::unbind() const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

}  // namespace sponge::platform::opengl
