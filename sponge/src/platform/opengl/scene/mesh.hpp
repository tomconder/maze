#pragma once

#include "modeldata.hpp"
#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/renderer/texture.hpp"
#include "scene/clustercut.hpp"
#include "scene/frustum.hpp"
#include "vertex.hpp"

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace sponge::platform::opengl::scene {

using sponge::scene::MeshUVTransforms;
using sponge::scene::UVTransform;

// Which meshes a pass draws. A mesh belongs to Opaque, Masked or Blended; the
// other values select a group. Solid is Opaque and Masked together: what the
// opaque colour pass draws. The depth-only passes draw Opaque and Masked with
// different programs, so a discard does not turn off early-Z for the opaque
// meshes. Blended meshes are drawn last, sorted, and cast no shadow.
enum class AlphaPass : uint8_t { All, Opaque, Masked, Solid, Blended };

class Mesh {
public:
    Mesh(
        std::vector<sponge::scene::PackedVertex>&& vertices,
        std::size_t numVertices, std::vector<uint32_t>&& indices,
        std::size_t                                       numIndices,
        std::vector<std::shared_ptr<renderer::Texture>>&& textures,
        std::shared_ptr<renderer::Texture> normalTexture              = nullptr,
        std::shared_ptr<renderer::Texture> occlusionTexture           = nullptr,
        std::shared_ptr<renderer::Texture> emissiveTexture            = nullptr,
        std::shared_ptr<renderer::Texture> metallicRoughnessTexture   = nullptr,
        std::shared_ptr<renderer::Texture> diffuseTransmissionTexture = nullptr,
        float metallicFactor = 0.F, float roughnessFactor = .5F,
        float clearcoatFactor = 0.F, float clearcoatRoughnessFactor = 0.F,
        const glm::vec4&         diffuseTransmission = { 1.F, 1.F, 1.F, 0.F },
        const glm::vec4&         baseColorFactor     = glm::vec4(1.F),
        sponge::scene::AlphaMode alphaMode = sponge::scene::AlphaMode::Opaque,
        float alphaCutoff = 0.F, bool doubleSided = false,
        const MeshUVTransforms& uvTransforms = {});
    // Sets the material and draws this mesh's range of the model's shared
    // vertex and index buffers. The caller binds the model's VAO first. The
    // PBR program always gets its full material. Any other program gets only
    // the alpha test inputs, and only when alphaTest is set: the masked depth
    // prepass and shadow programs use it, so their discards match the PBR
    // pass. ranges are the clusters to draw, relative to this mesh. Empty
    // draws LOD0. Returns the number of indices drawn.
    size_t draw(const std::shared_ptr<renderer::Shader>&   shader,
                bool                                       alphaTest = false,
                std::span<const sponge::scene::IndexRange> ranges = {}) const;

    // Debug view: the PBR program colors each range by its cluster depth.
    // Render thread only.
    static void setLodTint(const bool enabled) {
        lodTint = enabled;
    }
    static bool getLodTint() {
        return lodTint;
    }

    bool isDoubleSided() const {
        return doubleSided;
    }

    // Opaque, Masked (glTF MASK with a cutoff) or Blended.
    AlphaPass alphaPass() const {
        if (alphaMode == sponge::scene::AlphaMode::Blend) {
            return AlphaPass::Blended;
        }
        return alphaMode == sponge::scene::AlphaMode::Mask &&
                       alphaCutoff > 0.F ?
                   AlphaPass::Masked :
                   AlphaPass::Opaque;
    }

    // True if a pass that selects `selected` draws this mesh.
    bool drawnIn(const AlphaPass selected) const {
        const auto own = alphaPass();
        switch (selected) {
            case AlphaPass::All:
                return true;
            case AlphaPass::Solid:
                return own != AlphaPass::Blended;
            default:
                return own == selected;
        }
    }

    size_t getNumIndices() const {
        return numIndices;
    }
    size_t getNumVertices() const {
        return numVertices;
    }

    // The geometry Model packs into its shared buffers, then frees.
    std::span<const sponge::scene::PackedVertex> getVertices() const {
        return vertices;
    }
    std::span<const uint32_t> getIndices() const {
        return indices;
    }
    void freeGeometry() {
        std::vector<sponge::scene::PackedVertex>().swap(vertices);
        std::vector<uint32_t>().swap(indices);
    }

    // Where Model put this mesh in its shared buffers: the vertex offset
    // added to every index, and the first index in elements.
    void setDrawRange(const int32_t vertexOffset, const uint32_t firstElement) {
        baseVertex = vertexOffset;
        firstIndex = firstElement;
    }
    // The cluster LOD hierarchy, in model space. Kept after freeGeometry():
    // the game reads it once to build its world-space copy.
    void setClusters(std::vector<sponge::scene::MeshCluster>&& meshClusters);
    std::span<const sponge::scene::MeshCluster> getClusters() const {
        return clusters;
    }
    // LOD0 is the first lod0IndexCount indices of this mesh's range.
    uint32_t getLod0IndexCount() const {
        return lod0IndexCount;
    }
    int32_t getBaseVertex() const {
        return baseVertex;
    }
    uint32_t getFirstIndex() const {
        return firstIndex;
    }

    // Null until initShader() runs. Every Mesh calls it, so a scene with no
    // models has to call it itself. GL thread only.
    static void initShader();

    static std::shared_ptr<renderer::Shader> getShader() {
        return defaultShader;
    }

    // Model-space AABB, computed once from the vertex positions at build
    // time.
    const sponge::scene::AABB& getBounds() const {
        return bounds;
    }

private:
    std::vector<sponge::scene::PackedVertex> vertices;
    std::vector<uint32_t>                    indices;
    size_t                                   numIndices  = 0;
    size_t                                   numVertices = 0;

    std::vector<sponge::scene::MeshCluster> clusters;
    uint32_t                                lod0IndexCount = 0;

    static constexpr std::string_view        shaderName = "mesh";
    static uint32_t                          meshProgramId;
    static inline bool                       lodTint = false;
    static std::shared_ptr<renderer::Shader> defaultShader;

    // The material uniforms draw() sets on every draw, found once.
    struct MaterialUniforms {
        using Handle = renderer::Shader::UniformHandle;
        Handle hasNoTexture;
        Handle albedoUV;
        Handle hasNormalMap;
        Handle normalUV;
        Handle hasAOMap;
        Handle occlusionUV;
        Handle hasEmissiveMap;
        Handle emissiveUV;
        Handle metallicFactor;
        Handle roughnessFactor;
        Handle clearcoatFactor;
        Handle clearcoatRoughnessFactor;
        Handle baseColorFactor;
        Handle alphaCutoff;
        Handle alphaBlend;
        Handle diffuseTransmission;
        Handle hasDiffuseTransmissionMap;
        Handle diffuseTransmissionUV;
        Handle hasMetallicRoughnessMap;
        Handle metallicRoughnessUV;
        Handle lodLevel;
    };
    static MaterialUniforms materialUniforms;

    int32_t  baseVertex = 0;
    uint32_t firstIndex = 0;

    std::vector<std::shared_ptr<renderer::Texture>> textures;
    std::shared_ptr<renderer::Texture>              normalTexture;
    std::shared_ptr<renderer::Texture>              occlusionTexture;
    std::shared_ptr<renderer::Texture>              emissiveTexture;
    std::shared_ptr<renderer::Texture>              metallicRoughnessTexture;
    std::shared_ptr<renderer::Texture>              diffuseTransmissionTexture;
    float                                           metallicFactor;
    float                                           roughnessFactor;
    float                                           clearcoatFactor;
    float                                           clearcoatRoughnessFactor;
    glm::vec4                                       diffuseTransmission;
    glm::vec4                                       baseColorFactor;
    sponge::scene::AlphaMode                        alphaMode;
    float                                           alphaCutoff;
    bool                                            doubleSided;
    MeshUVTransforms                                uvTransforms;
    sponge::scene::AABB                             bounds;
};

}  // namespace sponge::platform::opengl::scene
