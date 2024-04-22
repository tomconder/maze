#include "openglindexbuffer.hpp"
#include "core/log.hpp"
#include "platform/opengl/gl.hpp"

namespace sponge::platform::opengl {

OpenGLIndexBuffer::OpenGLIndexBuffer() {
    SPONGE_CORE_WARN("Not implemented");
}

OpenGLIndexBuffer::OpenGLIndexBuffer(const std::vector<unsigned int>& indices) {
    glGenBuffers(1, &id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t),
                 indices.data(), GL_DYNAMIC_DRAW);
}

OpenGLIndexBuffer::OpenGLIndexBuffer(const uint32_t size) {
    glGenBuffers(1, &id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size * sizeof(uint32_t), nullptr,
                 GL_DYNAMIC_DRAW);
}

OpenGLIndexBuffer::OpenGLIndexBuffer(const OpenGLIndexBuffer& indexBuffer)
    : Buffer(indexBuffer) {
    id = indexBuffer.id;
}

OpenGLIndexBuffer::OpenGLIndexBuffer(OpenGLIndexBuffer&& indexBuffer) noexcept {
    id = indexBuffer.id;
}

OpenGLIndexBuffer& OpenGLIndexBuffer::operator=(
    const OpenGLIndexBuffer& indexBuffer) {
    id = indexBuffer.id;
    return *this;
}

OpenGLIndexBuffer::~OpenGLIndexBuffer() {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &id);
}

void OpenGLIndexBuffer::init() {
    SPONGE_CORE_WARN("Not implemented");
}

void OpenGLIndexBuffer::init(const std::vector<uint32_t>& indices) {
    glGenBuffers(1, &id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t),
                 indices.data(), GL_DYNAMIC_DRAW);
}

void OpenGLIndexBuffer::update(const std::vector<uint32_t>& indices) const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0,
                    indices.size() * sizeof(uint32_t), indices.data());
}

void OpenGLIndexBuffer::bind() const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
}

void OpenGLIndexBuffer::unbind() const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

}  // namespace sponge::renderer
