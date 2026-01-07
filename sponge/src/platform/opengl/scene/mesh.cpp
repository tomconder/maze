#include "platform/opengl/scene/mesh.hpp"

#include "debug/profiler.hpp"
#include "logging/log.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"

#include <cstddef>
#include <memory>
#include <string_view>
#include <vector>

namespace {
inline constexpr std::string_view normal   = "normal";
inline constexpr std::string_view position = "position";
inline constexpr std::string_view texCoord = "texCoord";
}  // namespace

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;
using sponge::scene::Vertex;

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
        .vertexShaderPath   = "/shaders/pbr.vert.glsl",
        .fragmentShaderPath = "/shaders/pbr.frag.glsl",
    };
    const auto shader = AssetManager::createShader(shaderCreateInfo);
    shader->bind();

    vao = renderer::VertexArray::create();
    vao->bind();

    vbo = std::make_unique<renderer::VertexBuffer>(
        this->vertices.data(), numVertices * sizeof(Vertex));
    vbo->bind();

    const auto program = shader->getId();

    auto location = glGetAttribLocation(program, position.data());
    if (location != -1) {
        const auto pos = static_cast<uint32_t>(location);
        glEnableVertexAttribArray(pos);
        glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              reinterpret_cast<const void*>(
                                  offsetof(sponge::scene::Vertex, position)));
    }

    location = glGetAttribLocation(program, texCoord.data());
    if (location != -1) {
        const auto pos = static_cast<uint32_t>(location);
        glEnableVertexAttribArray(pos);
        glVertexAttribPointer(pos, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              reinterpret_cast<const void*>(
                                  offsetof(sponge::scene::Vertex, texCoords)));
    }

    location = glGetAttribLocation(program, normal.data());
    if (location != -1) {
        const auto pos = static_cast<uint32_t>(location);
        glEnableVertexAttribArray(pos);
        glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              reinterpret_cast<const void*>(
                                  offsetof(sponge::scene::Vertex, normal)));
    }

    ebo = std::make_unique<renderer::IndexBuffer>(
        this->indices.data(), numIndices * sizeof(uint32_t));
    ebo->bind();

    shader->unbind();
    vao->unbind();
}

void Mesh::render(const std::shared_ptr<renderer::Shader>& shader) const {
    SPONGE_PROFILE;

    vao->bind();

    if (shader->getName() == "mesh") {
        if (!textures.empty()) {
            shader->setInteger("texture_diffuse1", 0);
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
