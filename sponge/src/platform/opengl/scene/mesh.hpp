#pragma once

#include "modeldata.hpp"
#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/renderer/texture.hpp"
#include "scene/frustum.hpp"
#include "scene/mesh.hpp"

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

class Mesh : public sponge::scene::Mesh {
public:
    Mesh(
        std::vector<sponge::scene::Vertex>&& vertices, std::size_t numVertices,
        std::vector<uint32_t>&& indices, std::size_t numIndices,
        std::vector<std::shared_ptr<renderer::Texture>>&& textures,
        std::shared_ptr<renderer::Texture> normalTexture              = nullptr,
        std::shared_ptr<renderer::Texture> occlusionTexture           = nullptr,
        std::shared_ptr<renderer::Texture> emissiveTexture            = nullptr,
        std::shared_ptr<renderer::Texture> metallicRoughnessTexture   = nullptr,
        std::shared_ptr<renderer::Texture> diffuseTransmissionTexture = nullptr,
        float metallicFactor = 0.F, float roughnessFactor = .5F,
        float clearcoatFactor = 0.F, float clearcoatRoughnessFactor = 0.F,
        const glm::vec4& diffuseTransmission = { 1.F, 1.F, 1.F, 0.F },
        const glm::vec4& baseColorFactor     = glm::vec4(1.F),
        float alphaCutoff = 0.F, const MeshUVTransforms& uvTransforms = {});
    // Sets the material and draws this mesh's range of the model's shared
    // vertex and index buffers. The caller binds the model's VAO first. The
    // PBR program always gets its full material. Any other program gets only
    // the alpha test inputs, and only when alphaTest is set: the masked depth
    // prepass and shadow programs use it, so their discards match the PBR
    // pass.
    void draw(const std::shared_ptr<renderer::Shader>& shader,
              bool                                     alphaTest = false) const;

    // glTF MASK: the mesh needs the alpha test.
    bool isMasked() const {
        return alphaCutoff > 0.F;
    }

    // The geometry Model packs into its shared buffers, then frees.
    std::span<const sponge::scene::Vertex> getVertices() const {
        return vertices;
    }
    std::span<const uint32_t> getIndices() const {
        return indices;
    }
    void freeGeometry() {
        std::vector<sponge::scene::Vertex>().swap(vertices);
        std::vector<uint32_t>().swap(indices);
    }

    // Where Model put this mesh in its shared buffers: the vertex offset
    // added to every index, and the first index in elements.
    void setDrawRange(const int32_t vertexOffset, const uint32_t firstElement) {
        baseVertex = vertexOffset;
        firstIndex = firstElement;
    }
    int32_t getBaseVertex() const {
        return baseVertex;
    }
    uint32_t getFirstIndex() const {
        return firstIndex;
    }

    static std::shared_ptr<renderer::Shader> getShader() {
        return defaultShader;
    }

    // Model-space AABB, computed once from the vertex positions at build
    // time.
    const sponge::scene::AABB& getBounds() const {
        return bounds;
    }

private:
    static constexpr std::string_view        shaderName = "mesh";
    static uint32_t                          meshProgramId;
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
        Handle diffuseTransmission;
        Handle hasDiffuseTransmissionMap;
        Handle diffuseTransmissionUV;
        Handle hasMetallicRoughnessMap;
        Handle metallicRoughnessUV;
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
    float                                           alphaCutoff;
    MeshUVTransforms                                uvTransforms;
    sponge::scene::AABB                             bounds;
};

}  // namespace sponge::platform::opengl::scene
