#include "platform/opengl/scene/mesh.hpp"

#include "debug/profiler.hpp"
#include "logging/log.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace {
inline const std::string normal   = "normal";
inline const std::string position = "position";
inline const std::string texCoord = "texCoord";
}  // namespace

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;
using sponge::scene::Vertex;

inline const std::string Mesh::shaderName = "mesh";

Mesh::Mesh(std::vector<Vertex>&& vertices, const std::size_t numVertices,
           std::vector<uint32_t>&& indices, const std::size_t numIndices,
           std::vector<std::shared_ptr<renderer::Texture>>&& textures) :
    textures(std::move(textures)) {
    this->indices     = std::move(indices);
    this->numIndices  = numIndices;
    this->vertices    = std::move(vertices);
    this->numVertices = numVertices;

    const auto shaderCreateInfo = renderer::ShaderCreateInfo{
        .name               = shaderName,
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

    auto location = glGetAttribLocation(program, position.c_str());
    if (location != -1) {
        const auto pos = static_cast<uint32_t>(location);
        glEnableVertexAttribArray(pos);
        glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              reinterpret_cast<const void*>(
                                  offsetof(sponge::scene::Vertex, position)));
    }

    location = glGetAttribLocation(program, texCoord.c_str());
    if (location != -1) {
        const auto pos = static_cast<uint32_t>(location);
        glEnableVertexAttribArray(pos);
        glVertexAttribPointer(pos, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              reinterpret_cast<const void*>(
                                  offsetof(sponge::scene::Vertex, texCoords)));
    }

    location = glGetAttribLocation(program, normal.c_str());
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
