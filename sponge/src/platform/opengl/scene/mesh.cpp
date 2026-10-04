#include "platform/opengl/scene/mesh.hpp"

#include "debug/profiler.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"

#include <glm/glm.hpp>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace {
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

std::shared_ptr<Shader> Mesh::defaultShader;

Mesh::MaterialUniforms Mesh::materialUniforms;

Mesh::Mesh(std::vector<Vertex>&& vertices, const std::size_t numVertices,
           std::vector<uint32_t>&& indices, const std::size_t numIndices,
           std::vector<std::shared_ptr<renderer::Texture>>&& textures,
           std::shared_ptr<renderer::Texture>                normalTexture,
           std::shared_ptr<renderer::Texture>                occlusionTexture,
           std::shared_ptr<renderer::Texture>                emissiveTexture,
           std::shared_ptr<renderer::Texture> metallicRoughnessTexture,
           const float metallicFactor, const float roughnessFactor,
           const float clearcoatFactor, const float clearcoatRoughnessFactor,
           const MeshUVTransforms& uvTransforms) :
    textures(std::move(textures)),
    normalTexture(std::move(normalTexture)),
    occlusionTexture(std::move(occlusionTexture)),
    emissiveTexture(std::move(emissiveTexture)),
    metallicRoughnessTexture(std::move(metallicRoughnessTexture)),
    metallicFactor(metallicFactor),
    roughnessFactor(roughnessFactor),
    clearcoatFactor(clearcoatFactor),
    clearcoatRoughnessFactor(clearcoatRoughnessFactor),
    uvTransforms(uvTransforms) {
    this->indices     = std::move(indices);
    this->numIndices  = numIndices;
    this->vertices    = std::move(vertices);
    this->numVertices = numVertices;

    glm::vec3 boundsMin(std::numeric_limits<float>::max());
    glm::vec3 boundsMax(std::numeric_limits<float>::lowest());
    for (const auto& vertex : this->vertices) {
        boundsMin = glm::min(boundsMin, vertex.position);
        boundsMax = glm::max(boundsMax, vertex.position);
    }
    bounds = { boundsMin, boundsMax };

    const auto shaderCreateInfo = renderer::ShaderCreateInfo{
        .name           = shaderName.data(),
        .vertexShader   = "pbr.vert",
        .fragmentShader = "pbr.frag",
    };
    defaultShader    = AssetManager::createShader(shaderCreateInfo);
    meshProgramId    = defaultShader->getId();
    materialUniforms = {
        .hasNoTexture    = defaultShader->findUniform("hasNoTexture"),
        .albedoUV        = defaultShader->findUniform("albedoUVTransform"),
        .hasNormalMap    = defaultShader->findUniform("hasNormalMap"),
        .normalUV        = defaultShader->findUniform("normalUVTransform"),
        .hasAOMap        = defaultShader->findUniform("hasAOMap"),
        .occlusionUV     = defaultShader->findUniform("occlusionUVTransform"),
        .hasEmissiveMap  = defaultShader->findUniform("hasEmissiveMap"),
        .emissiveUV      = defaultShader->findUniform("emissiveUVTransform"),
        .metallicFactor  = defaultShader->findUniform("metallicFactor"),
        .roughnessFactor = defaultShader->findUniform("roughnessFactor"),
        .clearcoatFactor = defaultShader->findUniform("clearcoatFactor"),
        .clearcoatRoughnessFactor =
            defaultShader->findUniform("clearcoatRoughnessFactor"),
        .hasMetallicRoughnessMap =
            defaultShader->findUniform("hasMetallicRoughnessMap"),
        .metallicRoughnessUV =
            defaultShader->findUniform("metallicRoughnessUVTransform"),
    };
}

void Mesh::draw(const std::shared_ptr<Shader>& shader) const {
    SPONGE_PROFILE;

    if (shader->getId() == meshProgramId) {
        shader->beginBatch();
        const auto& u     = materialUniforms;
        const auto  setUV = [&shader](const Shader::UniformHandle handle,
                                      const UVTransform&          uv) {
            shader->setFloat4(handle, glm::vec4(uv.offset, uv.scale));
        };

        if (!textures.empty()) {
            shader->setBoolean(u.hasNoTexture, false);
            textures.at(0)->bind(0);
        } else {
            shader->setBoolean(u.hasNoTexture, true);
        }
        setUV(u.albedoUV, uvTransforms.albedo);

        if (normalTexture) {
            shader->setBoolean(u.hasNormalMap, true);
            normalTexture->bind(normalTextureUnit);
        } else {
            shader->setBoolean(u.hasNormalMap, false);
        }
        setUV(u.normalUV, uvTransforms.normal);

        if (occlusionTexture) {
            shader->setBoolean(u.hasAOMap, true);
            occlusionTexture->bind(occlusionTextureUnit);
        } else {
            shader->setBoolean(u.hasAOMap, false);
        }
        setUV(u.occlusionUV, uvTransforms.occlusion);

        if (emissiveTexture) {
            shader->setBoolean(u.hasEmissiveMap, true);
            emissiveTexture->bind(emissiveTextureUnit);
        } else {
            shader->setBoolean(u.hasEmissiveMap, false);
        }
        setUV(u.emissiveUV, uvTransforms.emissive);

        shader->setFloat(u.metallicFactor, metallicFactor);
        shader->setFloat(u.roughnessFactor, roughnessFactor);
        shader->setFloat(u.clearcoatFactor, clearcoatFactor);
        shader->setFloat(u.clearcoatRoughnessFactor, clearcoatRoughnessFactor);
        if (metallicRoughnessTexture) {
            shader->setBoolean(u.hasMetallicRoughnessMap, true);
            metallicRoughnessTexture->bind(metallicRoughnessTextureUnit);
        } else {
            shader->setBoolean(u.hasMetallicRoughnessMap, false);
        }
        setUV(u.metallicRoughnessUV, uvTransforms.metallicRoughness);
        shader->endBatch();
    }

    glDrawElementsBaseVertex(
        GL_TRIANGLES, static_cast<GLsizei>(numIndices), GL_UNSIGNED_INT,
        reinterpret_cast<const void*>(static_cast<std::uintptr_t>(firstIndex) *
                                      sizeof(uint32_t)),
        baseVertex);
}
}  // namespace sponge::platform::opengl::scene
