#pragma once

#include "platform/opengl/openglmesh.h"
#include "platform/opengl/opengltexture.h"
#include <tiny_obj_loader.h>
#include <vector>

namespace sponge::renderer {

class OpenGLModel {
   public:
    void load(std::string_view path);
    void render() const;
    size_t getNumIndices() const {
        return numIndices;
    }
    size_t getNumVertices() const {
        return numVertices;
    }

   protected:
    std::vector<std::shared_ptr<OpenGLMesh>> meshes;

   private:
    size_t numIndices = 0;
    size_t numVertices = 0;

    void process(tinyobj::attrib_t& attrib,
                 std::vector<tinyobj::shape_t>& shapes,
                 const std::vector<tinyobj::material_t>& materials);
    static std::shared_ptr<OpenGLMesh> processMesh(
        tinyobj::attrib_t& attrib, tinyobj::mesh_t& mesh,
        const std::vector<tinyobj::material_t>& materials);
    static std::shared_ptr<OpenGLTexture> loadMaterialTextures(
        const tinyobj::material_t& material);
};

}  // namespace sponge::renderer
