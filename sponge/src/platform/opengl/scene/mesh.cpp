#include "mesh.hpp"
#include "platform/opengl/renderer/resourcemanager.hpp"
#include <cstddef>

namespace {
constexpr char normal[] = "normal";
constexpr char position[] = "position";
constexpr char texCoord[] = "texCoord";
}  // namespace

namespace sponge::platform::opengl::scene {

using renderer::ResourceManager;
using sponge::scene::Vertex;

Mesh::Mesh(const std::vector<Vertex>& vertices,
           const std::vector<uint32_t>& indices,
           const std::vector<std::shared_ptr<renderer::Texture>>& textures)
    : textures(textures) {
    this->indices = indices;
    this->vertices = vertices;

    shader = ResourceManager::loadShader(shaderName, "/shaders/shader.vert",
                                         "/shaders/shader.frag",
                                         "/shaders/shader.geom");
    shader->bind();

    vao = renderer::VertexArray::create();
    vao->bind();

    vbo = std::make_unique<renderer::VertexBuffer>(
        vertices.data(), vertices.size() * sizeof(Vertex));
    vbo->bind();

    const auto program = shader->getId();

    auto location = glGetAttribLocation(program, position);
    if (location != -1) {
        const auto pos = static_cast<uint32_t>(location);
        glEnableVertexAttribArray(pos);
        glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              reinterpret_cast<const void*>(
                                  offsetof(sponge::scene::Vertex, position)));
    }

    location = glGetAttribLocation(program, texCoord);
    if (location != -1) {
        const auto pos = static_cast<uint32_t>(location);
        glEnableVertexAttribArray(pos);
        glVertexAttribPointer(pos, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              reinterpret_cast<const void*>(
                                  offsetof(sponge::scene::Vertex, texCoords)));
    }

    location = glGetAttribLocation(program, normal);
    if (location != -1) {
        const auto pos = static_cast<uint32_t>(location);
        glEnableVertexAttribArray(pos);
        glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              reinterpret_cast<const void*>(
                                  offsetof(sponge::scene::Vertex, normal)));
    }

    ebo = renderer::IndexBuffer::create(indices);
    ebo->bind();

    shader->unbind();
    vao->unbind();
}

void Mesh::render() const {
    vao->bind();

    shader->bind();

    if (!textures.empty()) {
        shader->setInteger("texture_diffuse1", 0);
        textures[0]->bind();
    } else {
        shader->setBoolean("hasNoTexture", true);
    }

    glDrawElements(GL_TRIANGLES, static_cast<int32_t>(indices.size()),
                   GL_UNSIGNED_INT, nullptr);

    shader->unbind();

    vao->unbind();
}

}  // namespace sponge::platform::opengl::scene
