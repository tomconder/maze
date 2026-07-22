#pragma once

#include "platform/opengl/renderer/texture.hpp"
#include "platform/opengl/scene/mesh.hpp"

#include <glm/glm.hpp>
#include <cgltf.h>
#include <tiny_obj_loader.h>

#include <memory>
#include <string>
#include <vector>

namespace sponge::platform::opengl::scene {

struct ModelCreateInfo {
    std::string name;
    std::string path;
    std::string assetsFolder = core::File::getResourceDir();
};

class Model {
public:
    explicit Model(const ModelCreateInfo& createInfo);
    void   render(const std::shared_ptr<renderer::Shader>& shader) const;
    size_t getNumIndices() const {
        return numIndices;
    }
    size_t getNumVertices() const {
        return numVertices;
    }

protected:
    std::vector<std::shared_ptr<Mesh>> meshes;

private:
    size_t numIndices  = 0;
    size_t numVertices = 0;

    void load(const std::string& path);
    void loadObj(const std::string& path);
    void process(tinyobj::attrib_t&                      attrib,
                 std::vector<tinyobj::shape_t>&          shapes,
                 const std::vector<tinyobj::material_t>& materials,
                 const std::string&                      path);
    static std::shared_ptr<Mesh>
        processMesh(tinyobj::attrib_t& attrib, tinyobj::mesh_t& mesh,
                    const std::vector<tinyobj::material_t>& materials,
                    const std::string&                      path);
    static std::shared_ptr<renderer::Texture>
        loadMaterialTextures(const tinyobj::material_t& material,
                             const std::string&         path);

    void loadGltf(const std::string& path);
    static std::shared_ptr<Mesh>
                processGltfPrimitive(const cgltf_primitive& primitive,
                                     const glm::mat4&       transform,
                                     const std::string&     path);
    static void computeTangents(std::vector<sponge::scene::Vertex>& vertices,
                                const std::vector<uint32_t>&        indices);
    static std::shared_ptr<renderer::Texture>
                       loadGltfTexture(const cgltf_texture_view& textureView,
                                       const std::string&        path);
    static UVTransform gltfUVTransform(const cgltf_texture_view& textureView);
};

}  // namespace sponge::platform::opengl::scene
