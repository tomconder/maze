#pragma once

#include <assimp/scene.h>

#include <memory>
#include <string>
#include <vector>

#include "openglmesh.h"
#include "opengltexture.h"
#include "renderer/mesh.h"

class OpenGLModel {
   public:
    void load(const std::string &path);
    void render();

    std::vector<OpenGLMesh> meshes;

   private:
    static OpenGLMesh processMesh(const aiMesh *mesh, const aiScene *scene);
    static std::vector<std::shared_ptr<OpenGLTexture>> loadMaterialTextures(const aiMaterial *mat,
                                                                            aiTextureType textureType,
                                                                            const std::string &typeName);
    void processNode(const aiNode *node, const aiScene *scene);
};
