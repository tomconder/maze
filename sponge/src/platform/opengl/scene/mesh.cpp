#include "platform/opengl/scene/mesh.hpp"

#include "debug/profiler.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"

#include <cstddef>
#include <memory>
#include <string_view>
#include <vector>

namespace {
// Slang-generated layout locations for pbr.vert.slang
constexpr uint32_t positionLoc = 0;
constexpr uint32_t texCoordLoc = 1;
constexpr uint32_t normalLoc   = 2;
}  // namespace

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;
using renderer::Shader;
using sponge::scene::Vertex;

uint32_t Mesh::meshProgramId = 0;

std::shared_ptr<renderer::Shader> Mesh::shader;

Mesh::Mesh(std::vector<Vertex>&& vertices, const std::size_t numVertices,
           std::vector<uint32_t>&& indices, const std::size_t numIndices,
           std::vector<std::shared_ptr<renderer::Texture>>&& textures) :
    textures(std::move(textures)) {
    this->indices     = std::move(indices);
    this->numIndices  = numIndices;
    this->vertices    = std::move(vertices);
    this->numVertices = numVertices;

    const auto shaderCreateInfo = renderer::ShaderCreateInfo{
        .name               = shaderName.data(),
        .vertexShaderPath   = "/shaders/glsl/pbr.vert.glsl",
        .fragmentShaderPath = "/shaders/glsl/pbr.frag.glsl",
    };
    shader        = AssetManager::createShader(shaderCreateInfo);
    meshProgramId = shader->getId();
    shader->bind();

    vao = renderer::VertexArray::create();
    vao->bind();

    vbo = std::make_unique<renderer::VertexBuffer>(
        this->vertices.data(), numVertices * sizeof(Vertex));
    vbo->bind();

    glEnableVertexAttribArray(positionLoc);
    glVertexAttribPointer(positionLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<const void*>(
                              offsetof(sponge::scene::Vertex, position)));

    glEnableVertexAttribArray(texCoordLoc);
    glVertexAttribPointer(texCoordLoc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<const void*>(
                              offsetof(sponge::scene::Vertex, texCoords)));

    glEnableVertexAttribArray(normalLoc);
    glVertexAttribPointer(
        normalLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        reinterpret_cast<const void*>(offsetof(sponge::scene::Vertex, normal)));

    ebo = std::make_unique<renderer::IndexBuffer>(
        this->indices.data(), numIndices * sizeof(uint32_t));
    ebo->bind();

    shader->unbind();
    vao->unbind();
}

void Mesh::render(const std::shared_ptr<Shader>& shader) const {
    SPONGE_PROFILE;

    vao->bind();

    if (shader->getId() == meshProgramId) {
        if (!textures.empty()) {
            shader->setBoolean("hasNoTexture", false);
            textures.at(0)->activateAndBind(0);
        } else {
            shader->setBoolean("hasNoTexture", true);
        }
    }

    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(numIndices),
                   GL_UNSIGNED_INT, nullptr);

    vao->unbind();
}
}  // namespace sponge::platform::opengl::scene
