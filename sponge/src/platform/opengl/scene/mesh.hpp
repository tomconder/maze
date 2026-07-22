#pragma once

#include "platform/opengl/renderer/indexbuffer.hpp"
#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/renderer/texture.hpp"
#include "platform/opengl/renderer/vertexarray.hpp"
#include "platform/opengl/renderer/vertexbuffer.hpp"
#include "scene/mesh.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <string_view>
#include <vector>

namespace sponge::platform::opengl::scene {

// KHR_texture_transform offset/scale (no rotation support; unused by our
// current gltf assets/gltfpack output).
struct UVTransform {
    glm::vec2 offset{ 0.F, 0.F };
    glm::vec2 scale{ 1.F, 1.F };
};

struct MeshUVTransforms {
    UVTransform albedo;
    UVTransform normal;
    UVTransform occlusion;
    UVTransform emissive;
    UVTransform metallicRoughness;
};

class Mesh : public sponge::scene::Mesh {
public:
    Mesh(std::vector<sponge::scene::Vertex>&& vertices, std::size_t numVertices,
         std::vector<uint32_t>&& indices, std::size_t numIndices,
         std::vector<std::shared_ptr<renderer::Texture>>&& textures,
         std::shared_ptr<renderer::Texture> normalTexture            = nullptr,
         std::shared_ptr<renderer::Texture> occlusionTexture         = nullptr,
         std::shared_ptr<renderer::Texture> emissiveTexture          = nullptr,
         std::shared_ptr<renderer::Texture> metallicRoughnessTexture = nullptr,
         float metallicFactor = 0.F, float roughnessFactor = .5F,
         const MeshUVTransforms& uvTransforms = {});
    void render(const std::shared_ptr<renderer::Shader>& shader) const;

    static std::shared_ptr<renderer::Shader> getShader() {
        return shader;
    }

private:
    static constexpr std::string_view        shaderName = "mesh";
    static uint32_t                          meshProgramId;
    static std::shared_ptr<renderer::Shader> shader;

    std::unique_ptr<renderer::VertexBuffer> vbo;
    std::unique_ptr<renderer::IndexBuffer>  ebo;
    std::unique_ptr<renderer::VertexArray>  vao;

    std::vector<std::shared_ptr<renderer::Texture>> textures;
    std::shared_ptr<renderer::Texture>              normalTexture;
    std::shared_ptr<renderer::Texture>              occlusionTexture;
    std::shared_ptr<renderer::Texture>              emissiveTexture;
    std::shared_ptr<renderer::Texture>              metallicRoughnessTexture;
    float                                           metallicFactor;
    float                                           roughnessFactor;
    MeshUVTransforms                                uvTransforms;
};

}  // namespace sponge::platform::opengl::scene
