#pragma once

#include "platform/opengl/renderer/texture.hpp"
#include "platform/opengl/scene/mesh.hpp"
#include <tiny_obj_loader.h>
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
    void render(std::shared_ptr<renderer::Shader>& shader) const;
    size_t getNumIndices() const {
        return numIndices;
    }
    size_t getNumVertices() const {
        return numVertices;
    }

protected:
    std::vector<std::shared_ptr<Mesh>> meshes;

private:
    size_t numIndices = 0;
    size_t numVertices = 0;

    void load(const std::string& path);
    void process(tinyobj::attrib_t& attrib,
                 std::vector<tinyobj::shape_t>& shapes,
                 const std::vector<tinyobj::material_t>& materials,
                 const std::string& path);
    static std::shared_ptr<Mesh> processMesh(
        tinyobj::attrib_t& attrib, tinyobj::mesh_t& mesh,
        const std::vector<tinyobj::material_t>& materials,
        const std::string& path);
    static std::shared_ptr<renderer::Texture> loadMaterialTextures(
        const tinyobj::material_t& material, const std::string& path);
};

}  // namespace sponge::platform::opengl::scene
