#include "vertexbuffer.hpp"
#include "core/log.hpp"
#include "platform/opengl/gl.hpp"

namespace sponge::platform::opengl {

VertexBuffer::VertexBuffer() {
    glGenBuffers(1, &id);
    glBindBuffer(GL_ARRAY_BUFFER, id);
}

VertexBuffer::VertexBuffer(const std::vector<glm::vec2>& vertices) {
    glGenBuffers(1, &id);
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec2),
                 vertices.data(), GL_DYNAMIC_DRAW);
}

VertexBuffer::VertexBuffer(const std::vector<renderer::Vertex>& vertices) {
    glGenBuffers(1, &id);
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(renderer::Vertex),
                 vertices.data(), GL_DYNAMIC_DRAW);
}

VertexBuffer::VertexBuffer(const uint32_t size) {
    glGenBuffers(1, &id);
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferData(GL_ARRAY_BUFFER, size * sizeof(glm::vec2), nullptr,
                 GL_DYNAMIC_DRAW);
}

VertexBuffer::VertexBuffer(const VertexBuffer& vertexBuffer)
    : Buffer(vertexBuffer) {
    id = vertexBuffer.id;
}

VertexBuffer::VertexBuffer(VertexBuffer&& vertexBuffer) noexcept {
    id = vertexBuffer.id;
}

VertexBuffer& VertexBuffer::operator=(const VertexBuffer& vertexBuffer) {
    id = vertexBuffer.id;
    return *this;
}

VertexBuffer::~VertexBuffer() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &id);
}

void VertexBuffer::init() {
    SPONGE_CORE_WARN("Not implemented");
}

void VertexBuffer::init(const float* data, uint32_t size) const {
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
}

void VertexBuffer::update(const std::vector<glm::vec2>& vertices) const {
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(glm::vec2),
                    vertices.data());
}

void VertexBuffer::update(const std::vector<renderer::Vertex>& vertices) const {
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    vertices.size() * sizeof(renderer::Vertex),
                    vertices.data());
}

void VertexBuffer::bind() const {
    glBindBuffer(GL_ARRAY_BUFFER, id);
}

void VertexBuffer::unbind() const {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

}  // namespace sponge::platform::opengl
