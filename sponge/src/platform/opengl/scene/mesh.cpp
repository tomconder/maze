#include "mesh.hpp"
#include "debug/profiler.hpp"
#include "logging/log.hpp"
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

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::size_t numVertices,
           const std::vector<uint32_t>& indices, const std::size_t numIndices,
           const std::vector<std::shared_ptr<renderer::Texture>>& textures)
    : textures(textures) {
    this->indices = indices;
    this->numIndices = numIndices;
    this->vertices = vertices;
    this->numVertices = numVertices;

    const auto shaderCreateInfo = renderer::ShaderCreateInfo{
        .name = shaderName,
        .vertexShaderPath = "/shaders/pbr.vert.glsl",
        .fragmentShaderPath = "/shaders/pbr.frag.glsl",
    };
    const auto shader = ResourceManager::loadShader(shaderCreateInfo);
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

    ebo = std::make_unique<renderer::IndexBuffer>(
        indices.data(), numIndices * sizeof(uint32_t));
    ebo->bind();

    shader->unbind();
    vao->unbind();
}

void Mesh::render(std::shared_ptr<renderer::Shader>& shader) const {
    SPONGE_PROFILE;

    vao->bind();

    if (shader->getName() == "mesh") {
        if (!textures.empty()) {
            shader->setInteger("texture_diffuse1", 0);
            shader->setBoolean("hasNoTexture", false);
            textures[0]->activateAndBind(0);
        } else {
            shader->setBoolean("hasNoTexture", true);
        }
    }

    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(numIndices),
                   GL_UNSIGNED_INT, nullptr);

    vao->unbind();
}
}  // namespace sponge::platform::opengl::scene
