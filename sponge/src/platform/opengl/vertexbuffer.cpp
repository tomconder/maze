#include "vertexbuffer.hpp"
#include "platform/opengl/gl.hpp"

namespace sponge::platform::opengl {

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

VertexBuffer::~VertexBuffer() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &id);
}

std::unique_ptr<VertexBuffer> VertexBuffer::create(const uint32_t size) {
    std::unique_ptr<VertexBuffer> buffer(new VertexBuffer(size));
    return buffer;
}

std::unique_ptr<VertexBuffer> VertexBuffer::create(
    const std::vector<glm::vec2>& vertices) {
    std::unique_ptr<VertexBuffer> buffer(new VertexBuffer(vertices));
    return buffer;
}

std::unique_ptr<VertexBuffer> VertexBuffer::create(
    const std::vector<renderer::Vertex>& vertices) {
    std::unique_ptr<VertexBuffer> buffer(new VertexBuffer(vertices));
    return buffer;
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
