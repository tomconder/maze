#include "platform/opengl/openglmesh.hpp"
#include "platform/opengl/gl.hpp"
#include "platform/opengl/openglresourcemanager.hpp"
#include <cstddef>

namespace sponge::renderer {

constexpr std::string_view normal = "normal";
constexpr std::string_view position = "position";
constexpr std::string_view texCoord = "texCoord";

OpenGLMesh::OpenGLMesh(
    const std::string& shaderName, const std::vector<Vertex>& vertices,
    const std::vector<uint32_t>& indices,
    const std::vector<std::shared_ptr<OpenGLTexture>>& textures)
    : meshShader(shaderName), textures(textures) {
    assert(!shaderName.empty());

    this->indices = indices;
    this->vertices = vertices;

    const auto shader = OpenGLResourceManager::getShader(meshShader);
    shader->bind();

    vao = std::make_unique<OpenGLVertexArray>();
    vao->bind();

    vbo = std::make_unique<OpenGLBuffer>(
        reinterpret_cast<const float*>(vertices.data()),
        static_cast<uint32_t>(vertices.size()) *
            static_cast<uint32_t>(sizeof(Vertex)));
    vbo->bind();

    const auto program = shader->getId();

    auto location = glGetAttribLocation(program, position.data());
    if (location != -1) {
        const auto position = static_cast<uint32_t>(location);
        glEnableVertexAttribArray(position);
        glVertexAttribPointer(
            position, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            reinterpret_cast<const void*>(offsetof(Vertex, position)));
    }

    location = glGetAttribLocation(program, texCoord.data());
    if (location != -1) {
        const auto position = static_cast<uint32_t>(location);
        glEnableVertexAttribArray(position);
        glVertexAttribPointer(
            position, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            reinterpret_cast<const void*>(offsetof(Vertex, texCoords)));
    }

    location = glGetAttribLocation(program, normal.data());
    if (location != -1) {
        const auto position = static_cast<uint32_t>(location);
        glEnableVertexAttribArray(position);
        glVertexAttribPointer(
            position, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            reinterpret_cast<const void*>(offsetof(Vertex, normal)));
    }

    ebo = std::make_unique<OpenGLElementBuffer>(
        indices.data(), static_cast<uint32_t>(indices.size()) *
                            static_cast<uint32_t>(sizeof(unsigned int)));
    ebo->bind();

    shader->unbind();
    glBindVertexArray(0);
}

void OpenGLMesh::render() const {
    vao->bind();

    const auto shader = OpenGLResourceManager::getShader(meshShader);
    shader->bind();

    if (!textures.empty()) {
        shader->setInteger("texture_diffuse1", 0);
        textures[0]->bind();
    } else {
        shader->setBoolean("hasNoTexture", true);
    }

    glDrawElements(GL_TRIANGLES, static_cast<int32_t>(indices.size()),
                   GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

}  // namespace sponge::renderer
