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
// 10 to 13 are the SSAO, scene copy, probe and depth units mazelayer binds.
constexpr uint8_t diffuseTransmissionTextureUnit = 14;
}  // namespace

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;
using renderer::Shader;
using sponge::scene::PackedVertex;

uint32_t Mesh::meshProgramId = 0;

std::shared_ptr<Shader> Mesh::defaultShader;

Mesh::MaterialUniforms Mesh::materialUniforms;

Mesh::Mesh(std::vector<PackedVertex>&& vertices, const std::size_t numVertices,
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
        .lodLevel = defaultShader->findUniform("lodLevel"),
    };
    lod0IndexCount = static_cast<uint32_t>(numIndices);
}

void Mesh::setClusters(std::vector<sponge::scene::MeshCluster>&& meshClusters) {
    clusters       = std::move(meshClusters);
    lod0IndexCount = static_cast<uint32_t>(numIndices);
    if (!clusters.empty()) {
        lod0IndexCount = 0;
        for (const auto& cluster : clusters) {
            lod0IndexCount += cluster.depth == 0 ? cluster.indexCount : 0;
        }
    }
}

size_t
    Mesh::draw(const std::shared_ptr<Shader>& shader, const bool alphaTest,
               const std::span<const sponge::scene::IndexRange> ranges) const {
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
        shader->setInteger(u.lodLevel, lodTint ? 0 : -1);
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

    const auto offset = [this](const uint32_t first) {
        return reinterpret_cast<const void*>(
            static_cast<std::uintptr_t>(firstIndex + first) * sizeof(uint32_t));
    };
    if (ranges.empty()) {
        glDrawElementsBaseVertex(GL_TRIANGLES,
                                 static_cast<GLsizei>(lod0IndexCount),
                                 GL_UNSIGNED_INT, offset(0), baseVertex);
        return lod0IndexCount;
    }

    size_t drawn = 0;
    if (lodTint && shader->getId() == meshProgramId) {
        // One draw per range, so each gets its own color.
        for (const auto& range : ranges) {
            shader->setInteger(materialUniforms.lodLevel,
                               static_cast<int>(range.depth));
            glDrawElementsBaseVertex(
                GL_TRIANGLES, static_cast<GLsizei>(range.indexCount),
                GL_UNSIGNED_INT, offset(range.firstIndex), baseVertex);
            drawn += range.indexCount;
        }
        return drawn;
    }

    // One call for the whole cut. Only the render thread draws, so the
    // scratch arrays are reused from draw to draw.
    thread_local std::vector<GLsizei>     counts;
    thread_local std::vector<const void*> offsets;
    thread_local std::vector<GLint>       baseVertices;
    counts.clear();
    offsets.clear();
    for (const auto& range : ranges) {
        counts.push_back(static_cast<GLsizei>(range.indexCount));
        offsets.push_back(offset(range.firstIndex));
        drawn += range.indexCount;
    }
    baseVertices.assign(ranges.size(), baseVertex);
    glMultiDrawElementsBaseVertex(
        GL_TRIANGLES, counts.data(), GL_UNSIGNED_INT, offsets.data(),
        static_cast<GLsizei>(ranges.size()), baseVertices.data());
    return drawn;
}
}  // namespace sponge::platform::opengl::scene
