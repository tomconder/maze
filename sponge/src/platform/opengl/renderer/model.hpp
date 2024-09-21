#pragma once

#include "platform/opengl/renderer/texture.hpp"
#include "platform/opengl/scene/mesh.hpp"
#include <tiny_obj_loader.h>
#include <vector>

namespace sponge::platform::opengl::renderer {

class Model {
   public:
    void load(const std::string& shaderName, const std::string& path);
    void render() const;
    size_t getNumIndices() const {
        return numIndices;
    }
    size_t getNumVertices() const {
        return numVertices;
    }

   protected:
    std::vector<std::shared_ptr<scene::Mesh>> meshes;

   private:
    size_t numIndices = 0;
    size_t numVertices = 0;

    void process(const std::string& shaderName, tinyobj::attrib_t& attrib,
                 std::vector<tinyobj::shape_t>& shapes,
                 const std::vector<tinyobj::material_t>& materials,
                 const std::string& path);
    static std::shared_ptr<scene::Mesh> processMesh(
        const std::string& shaderName, tinyobj::attrib_t& attrib,
        tinyobj::mesh_t& mesh,
        const std::vector<tinyobj::material_t>& materials,
        const std::string& path);
    static std::shared_ptr<Texture> loadMaterialTextures(
        const tinyobj::material_t& material, const std::string& path);
};

}  // namespace sponge::platform::opengl::renderer
