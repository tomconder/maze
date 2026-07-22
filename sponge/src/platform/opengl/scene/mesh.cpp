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
constexpr uint32_t tangentLoc  = 3;

// Texture units; 0/1 are reserved for albedo/shadow map, bound elsewhere.
constexpr uint8_t normalTextureUnit            = 6;
constexpr uint8_t occlusionTextureUnit         = 7;
constexpr uint8_t emissiveTextureUnit          = 8;
constexpr uint8_t metallicRoughnessTextureUnit = 9;
}  // namespace

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;
using renderer::Shader;
using sponge::scene::Vertex;

uint32_t Mesh::meshProgramId = 0;

std::shared_ptr<renderer::Shader> Mesh::shader;

Mesh::Mesh(std::vector<Vertex>&& vertices, const std::size_t numVertices,
           std::vector<uint32_t>&& indices, const std::size_t numIndices,
           std::vector<std::shared_ptr<renderer::Texture>>&& textures,
           std::shared_ptr<renderer::Texture>                normalTexture,
           std::shared_ptr<renderer::Texture>                occlusionTexture,
           std::shared_ptr<renderer::Texture>                emissiveTexture,
           std::shared_ptr<renderer::Texture> metallicRoughnessTexture,
           const float metallicFactor, const float roughnessFactor,
           const MeshUVTransforms& uvTransforms) :
    textures(std::move(textures)),
    normalTexture(std::move(normalTexture)),
    occlusionTexture(std::move(occlusionTexture)),
    emissiveTexture(std::move(emissiveTexture)),
    metallicRoughnessTexture(std::move(metallicRoughnessTexture)),
    metallicFactor(metallicFactor),
    roughnessFactor(roughnessFactor),
    uvTransforms(uvTransforms) {
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

    glEnableVertexAttribArray(tangentLoc);
    glVertexAttribPointer(tangentLoc, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<const void*>(
                              offsetof(sponge::scene::Vertex, tangent)));

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
        const auto setUV = [&shader](const std::string_view name,
                                     const UVTransform&     uv) {
            shader->setFloat4(name, glm::vec4(uv.offset, uv.scale));
        };

        if (!textures.empty()) {
            shader->setBoolean("hasNoTexture", false);
            textures.at(0)->activateAndBind(0);
        } else {
            shader->setBoolean("hasNoTexture", true);
        }
        setUV("albedoUVTransform", uvTransforms.albedo);

        if (normalTexture) {
            shader->setBoolean("hasNormalMap", true);
            normalTexture->activateAndBind(normalTextureUnit);
        } else {
            shader->setBoolean("hasNormalMap", false);
        }
        setUV("normalUVTransform", uvTransforms.normal);

        if (occlusionTexture) {
            shader->setBoolean("hasAOMap", true);
            occlusionTexture->activateAndBind(occlusionTextureUnit);
        } else {
            shader->setBoolean("hasAOMap", false);
        }
        setUV("occlusionUVTransform", uvTransforms.occlusion);

        if (emissiveTexture) {
            shader->setBoolean("hasEmissiveMap", true);
            emissiveTexture->activateAndBind(emissiveTextureUnit);
        } else {
            shader->setBoolean("hasEmissiveMap", false);
        }
        setUV("emissiveUVTransform", uvTransforms.emissive);

        shader->setFloat("metallicFactor", metallicFactor);
        shader->setFloat("roughnessFactor", roughnessFactor);
        if (metallicRoughnessTexture) {
            shader->setBoolean("hasMetallicRoughnessMap", true);
            metallicRoughnessTexture->activateAndBind(
                metallicRoughnessTextureUnit);
        } else {
            shader->setBoolean("hasMetallicRoughnessMap", false);
        }
        setUV("metallicRoughnessUVTransform", uvTransforms.metallicRoughness);
    }

    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(numIndices),
                   GL_UNSIGNED_INT, nullptr);

    vao->unbind();
}
}  // namespace sponge::platform::opengl::scene
