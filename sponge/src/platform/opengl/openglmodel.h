#pragma once

#include <memory>
#include <string>
#include <vector>

#include "openglmesh.h"
#include "opengltexture.h"
#include "renderer/mesh.h"

#define USE_TINYOBJ 1
#if USE_TINYOBJ
#include "tiny_obj_loader.h"
#endif

class OpenGLModel {
   public:
    void load(const std::string &path);
    void render();

    std::vector<OpenGLMesh> meshes;

   private:
    void process(tinyobj::attrib_t& attrib, std::vector<tinyobj::shape_t>& shapes,
                 const std::vector<tinyobj::material_t>& materials);
    static OpenGLMesh processMesh(tinyobj::attrib_t& attrib, tinyobj::mesh_t& mesh,
                                  const std::vector<tinyobj::material_t>& materials);
    static std::shared_ptr<OpenGLTexture> loadMaterialTextures(const tinyobj::material_t& material);
};
