#pragma once

#include "renderer/mesh.h"
#include "renderer/opengl/openglmesh.h"
#include "renderer/opengl/opengltexture.h"
#include <string>
#include <tiny_obj_loader.h>
#include <vector>

namespace Sponge {

class OpenGLModel {
   public:
    void load(std::string_view path);
    void render();

    std::vector<OpenGLMesh> meshes;

   private:
    void process(tinyobj::attrib_t& attrib, std::vector<tinyobj::shape_t>& shapes,
                 const std::vector<tinyobj::material_t>& materials);
    static OpenGLMesh processMesh(tinyobj::attrib_t& attrib, tinyobj::mesh_t& mesh,
                                  const std::vector<tinyobj::material_t>& materials);
    static std::shared_ptr<OpenGLTexture> loadMaterialTextures(const tinyobj::material_t& material);
};

}  // namespace Sponge
