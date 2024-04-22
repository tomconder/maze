#include "openglvertexbuffer.hpp"
#include "core/log.hpp"
#include "platform/opengl/gl.hpp"

namespace sponge::renderer {

OpenGLVertexBuffer::OpenGLVertexBuffer() {
    glGenBuffers(1, &id);
    glBindBuffer(GL_ARRAY_BUFFER, id);
}

OpenGLVertexBuffer::OpenGLVertexBuffer(const std::vector<glm::vec2>& vertices) {
    glGenBuffers(1, &id);
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec2),
                 vertices.data(), GL_DYNAMIC_DRAW);
}

OpenGLVertexBuffer::OpenGLVertexBuffer(const std::vector<Vertex>& vertices) {
    glGenBuffers(1, &id);
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
                 vertices.data(), GL_DYNAMIC_DRAW);
}

OpenGLVertexBuffer::OpenGLVertexBuffer(const uint32_t size) {
    glGenBuffers(1, &id);
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferData(GL_ARRAY_BUFFER, size * sizeof(glm::vec2), nullptr,
                 GL_DYNAMIC_DRAW);
}

OpenGLVertexBuffer::OpenGLVertexBuffer(const OpenGLVertexBuffer& vertexBuffer)
    : Buffer(vertexBuffer) {
    id = vertexBuffer.id;
}

OpenGLVertexBuffer::OpenGLVertexBuffer(
    OpenGLVertexBuffer&& vertexBuffer) noexcept {
    id = vertexBuffer.id;
}

OpenGLVertexBuffer& OpenGLVertexBuffer::operator=(
    const OpenGLVertexBuffer& vertexBuffer) {
    id = vertexBuffer.id;
    return *this;
}

OpenGLVertexBuffer::~OpenGLVertexBuffer() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &id);
}

void OpenGLVertexBuffer::init() {
    SPONGE_CORE_WARN("Not implemented");
}

void OpenGLVertexBuffer::init(const float* data, uint32_t size) const {
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
}

void OpenGLVertexBuffer::update(const std::vector<glm::vec2>& vertices) const {
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(glm::vec2),
                    vertices.data());
}

void OpenGLVertexBuffer::update(const std::vector<Vertex>& vertices) const {
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex),
                    vertices.data());
}

void OpenGLVertexBuffer::bind() const {
    glBindBuffer(GL_ARRAY_BUFFER, id);
}

void OpenGLVertexBuffer::unbind() const {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

}  // namespace sponge::renderer
