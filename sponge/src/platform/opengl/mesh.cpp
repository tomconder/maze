#include "mesh.hpp"
#include "platform/opengl/gl.hpp"
#include "platform/opengl/resourcemanager.hpp"
#include <cstddef>

namespace sponge::platform::opengl {

constexpr std::string_view normal = "normal";
constexpr std::string_view position = "position";
constexpr std::string_view texCoord = "texCoord";

Mesh::Mesh(
    const std::string& shaderName,
    const std::vector<renderer::Vertex>& vertices,
    const std::vector<uint32_t>& indices,
    const std::vector<std::shared_ptr<Texture>>& textures)
    : shaderName(shaderName), textures(textures) {
    assert(!shaderName.empty());

    this->indices = indices;
    this->vertices = vertices;

    const auto shader = ResourceManager::getShader(shaderName);
    shader->bind();

    vao = std::make_unique<VertexArray>();
    vao->bind();

    vbo = std::make_unique<VertexBuffer>(vertices);
    vbo->bind();

    const auto program = shader->getId();

    auto location = glGetAttribLocation(program, position.data());
    if (location != -1) {
        const auto position = static_cast<uint32_t>(location);
        glEnableVertexAttribArray(position);
        glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE,
                              sizeof(renderer::Vertex),
                              reinterpret_cast<const void*>(
                                  offsetof(renderer::Vertex, position)));
    }

    location = glGetAttribLocation(program, texCoord.data());
    if (location != -1) {
        const auto position = static_cast<uint32_t>(location);
        glEnableVertexAttribArray(position);
        glVertexAttribPointer(position, 2, GL_FLOAT, GL_FALSE,
                              sizeof(renderer::Vertex),
                              reinterpret_cast<const void*>(
                                  offsetof(renderer::Vertex, texCoords)));
    }

    location = glGetAttribLocation(program, normal.data());
    if (location != -1) {
        const auto position = static_cast<uint32_t>(location);
        glEnableVertexAttribArray(position);
        glVertexAttribPointer(
            position, 3, GL_FLOAT, GL_FALSE, sizeof(renderer::Vertex),
            reinterpret_cast<const void*>(offsetof(renderer::Vertex, normal)));
    }

    ebo = std::make_unique<IndexBuffer>(indices);
    ebo->bind();

    shader->unbind();
    glBindVertexArray(0);
}

void Mesh::render() const {
    vao->bind();

    const auto shader = ResourceManager::getShader(shaderName);
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

}  // namespace sponge::platform::opengl
