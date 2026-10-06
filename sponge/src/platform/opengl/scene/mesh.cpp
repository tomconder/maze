#include "platform/opengl/scene/mesh.hpp"

#include "debug/profiler.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"

#include <glm/glm.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <numeric>
#include <string_view>
#include <utility>
#include <vector>

namespace {
// Texture units; 0/1 are reserved for albedo/shadow map, bound elsewhere.
constexpr uint8_t normalTextureUnit            = 6;
constexpr uint8_t occlusionTextureUnit         = 7;
constexpr uint8_t emissiveTextureUnit          = 8;
constexpr uint8_t metallicRoughnessTextureUnit = 9;
// 10 to 13 are the SSAO, scene copy, probe and depth units mazelayer binds.
constexpr uint8_t diffuseTransmissionTextureUnit = 14;
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
           std::shared_ptr<renderer::Texture> diffuseTransmissionTexture,
           const float metallicFactor, const float roughnessFactor,
           const float clearcoatFactor, const float clearcoatRoughnessFactor,
           const glm::vec4&               diffuseTransmission,
           const glm::vec4&               baseColorFactor,
           const sponge::scene::AlphaMode alphaMode, const float alphaCutoff,
           const bool doubleSided, const MeshUVTransforms& uvTransforms) :
    textures(std::move(textures)),
    normalTexture(std::move(normalTexture)),
    occlusionTexture(std::move(occlusionTexture)),
    emissiveTexture(std::move(emissiveTexture)),
    metallicRoughnessTexture(std::move(metallicRoughnessTexture)),
    diffuseTransmissionTexture(std::move(diffuseTransmissionTexture)),
    metallicFactor(metallicFactor),
    roughnessFactor(roughnessFactor),
    clearcoatFactor(clearcoatFactor),
    clearcoatRoughnessFactor(clearcoatRoughnessFactor),
    diffuseTransmission(diffuseTransmission),
    baseColorFactor(baseColorFactor),
    alphaMode(alphaMode),
    alphaCutoff(alphaCutoff),
    doubleSided(doubleSided),
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
        .baseColorFactor = defaultShader->findUniform("baseColorFactor"),
        .alphaCutoff     = defaultShader->findUniform("alphaCutoff"),
        .alphaBlend      = defaultShader->findUniform("alphaBlend"),
        .diffuseTransmission =
            defaultShader->findUniform("diffuseTransmission"),
        .hasDiffuseTransmissionMap =
            defaultShader->findUniform("hasDiffuseTransmissionMap"),
        .diffuseTransmissionUV =
            defaultShader->findUniform("diffuseTransmissionUVTransform"),
        .hasMetallicRoughnessMap =
            defaultShader->findUniform("hasMetallicRoughnessMap"),
        .metallicRoughnessUV =
            defaultShader->findUniform("metallicRoughnessUVTransform"),
    };
}

void Mesh::draw(const std::shared_ptr<Shader>& shader,
                const bool                     alphaTest) const {
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
        shader->setFloat4(u.baseColorFactor, baseColorFactor);
        shader->setFloat(u.alphaCutoff, alphaCutoff);
        shader->setBoolean(u.alphaBlend,
                           alphaMode == sponge::scene::AlphaMode::Blend);
        shader->setFloat4(u.diffuseTransmission, diffuseTransmission);
        if (diffuseTransmissionTexture) {
            shader->setBoolean(u.hasDiffuseTransmissionMap, true);
            diffuseTransmissionTexture->bind(diffuseTransmissionTextureUnit);
        } else {
            shader->setBoolean(u.hasDiffuseTransmissionMap, false);
        }
        setUV(u.diffuseTransmissionUV, uvTransforms.diffuseTransmission);
        if (metallicRoughnessTexture) {
            shader->setBoolean(u.hasMetallicRoughnessMap, true);
            metallicRoughnessTexture->bind(metallicRoughnessTextureUnit);
        } else {
            shader->setBoolean(u.hasMetallicRoughnessMap, false);
        }
        setUV(u.metallicRoughnessUV, uvTransforms.metallicRoughness);
        shader->endBatch();
    } else if (alphaTest) {
        // The same albedo sampler, UV transform and alpha as pbr.slang reads.
        shader->beginBatch();
        shader->setFloat("alphaCutoff", alphaCutoff);
        shader->setFloat("baseColorAlpha", baseColorFactor.a);
        shader->setBoolean("hasNoTexture", textures.empty());
        const auto& uv = uvTransforms.albedo;
        shader->setFloat4("albedoUVTransform", glm::vec4(uv.offset, uv.scale));
        if (!textures.empty()) {
            textures.at(0)->bind(0);
        }
        shader->endBatch();
    }

    glDrawElementsBaseVertex(
        GL_TRIANGLES, static_cast<GLsizei>(numIndices), GL_UNSIGNED_INT,
        reinterpret_cast<const void*>(static_cast<std::uintptr_t>(firstIndex) *
                                      sizeof(uint32_t)),
        baseVertex);
}

void Mesh::freeGeometry() {
    if (alphaPass() == AlphaPass::Blended) {
        triangleIndices = indices;
        triangleCentres.reserve(indices.size() / 3);
        for (size_t i = 0; i + 2 < indices.size(); i += 3) {
            triangleCentres.push_back((vertices[indices[i]].position +
                                       vertices[indices[i + 1]].position +
                                       vertices[indices[i + 2]].position) /
                                      3.F);
        }
    }
    std::vector<sponge::scene::Vertex>().swap(vertices);
    std::vector<uint32_t>().swap(indices);
}

void Mesh::sortTriangles(const renderer::IndexBuffer& ebo,
                         const glm::vec3&             eye) const {
    if (triangleCentres.empty() || sortedEye == eye) {
        return;
    }

    std::vector<float> distance(triangleCentres.size());
    for (size_t t = 0; t < distance.size(); t++) {
        const auto toEye = triangleCentres[t] - eye;
        distance[t]      = glm::dot(toEye, toEye);
    }
    std::vector<uint32_t> order(distance.size());
    std::iota(order.begin(), order.end(), 0U);
    std::ranges::stable_sort(order,
                             [&distance](const uint32_t a, const uint32_t b) {
                                 return distance[a] > distance[b];
                             });

    sortedIndices.resize(triangleIndices.size());
    for (size_t t = 0; t < order.size(); t++) {
        std::copy_n(triangleIndices.begin() + (order[t] * 3), 3,
                    sortedIndices.begin() + (t * 3));
    }
    ebo.update(static_cast<size_t>(firstIndex) * sizeof(uint32_t),
               sortedIndices.data(), sortedIndices.size() * sizeof(uint32_t));
    sortedEye = eye;
}
}  // namespace sponge::platform::opengl::scene
