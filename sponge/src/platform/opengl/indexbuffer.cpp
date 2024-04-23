#include "indexbuffer.hpp"
#include "core/log.hpp"
#include "platform/opengl/gl.hpp"

namespace sponge::platform::opengl {

IndexBuffer::IndexBuffer() {
    SPONGE_CORE_WARN("Not implemented");
}

IndexBuffer::IndexBuffer(const std::vector<unsigned int>& indices) {
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

IndexBuffer::IndexBuffer(const IndexBuffer& indexBuffer) : Buffer(indexBuffer) {
    id = indexBuffer.id;
}

IndexBuffer::IndexBuffer(IndexBuffer&& indexBuffer) noexcept {
    id = indexBuffer.id;
}

IndexBuffer& IndexBuffer::operator=(const IndexBuffer& indexBuffer) {
    id = indexBuffer.id;
    return *this;
}

IndexBuffer::~IndexBuffer() {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &id);
}

void IndexBuffer::init() {
    SPONGE_CORE_WARN("Not implemented");
}

void IndexBuffer::init(const std::vector<uint32_t>& indices) {
    glGenBuffers(1, &id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t),
                 indices.data(), GL_DYNAMIC_DRAW);
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
