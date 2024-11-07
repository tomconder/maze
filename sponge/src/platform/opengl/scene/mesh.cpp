#include "mesh.hpp"
#include "debug/profiler.hpp"
#include "platform/opengl/renderer/resourcemanager.hpp"
#include <cstddef>

namespace {
constexpr char color[] = "color";
constexpr char normal[] = "normal";
constexpr char position[] = "position";
constexpr char texCoord[] = "texCoord";
}  // namespace

namespace sponge::platform::opengl::scene {

using renderer::ResourceManager;
using sponge::scene::Vertex;

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::size_t numVertices,
           const std::vector<uint32_t>& indices, const std::size_t numIndices,
           const std::vector<std::shared_ptr<renderer::Texture>>& textures)
    : textures(textures) {
    this->indices = indices;
    this->numIndices = numIndices;
    this->vertices = vertices;
    this->numVertices = numVertices;

    shader = ResourceManager::loadShader(shaderName, "/shaders/pbr.vert.glsl",
                                         "/shaders/pbr.frag.glsl",
                                         "/shaders/pbr.geom.glsl");
    shader->bind();

    vao = renderer::VertexArray::create();
    vao->bind();

    vbo = std::make_unique<renderer::VertexBuffer>(
        vertices.data(), numVertices * sizeof(Vertex));
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

    location = glGetAttribLocation(program, color);
    if (location != -1) {
        const auto pos = static_cast<uint32_t>(location);
        glEnableVertexAttribArray(pos);
        glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              reinterpret_cast<const void*>(
                                  offsetof(sponge::scene::Vertex, color)));
    }

    ebo = std::make_unique<renderer::IndexBuffer>(
        indices.data(), numIndices * sizeof(uint32_t));
    ebo->bind();

    shader->unbind();
    vao->unbind();
}

void Mesh::render() const {
    SPONGE_PROFILE;

    vao->bind();

    shader->bind();

    if (!textures.empty()) {
        shader->setInteger("texture_diffuse1", 0);
        shader->setBoolean("hasNoTexture", false);
        textures[0]->bind();
    } else {
        shader->setBoolean("hasNoTexture", true);
    }

    glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, nullptr);

    shader->unbind();

    vao->unbind();
}

}  // namespace sponge::platform::opengl::scene
