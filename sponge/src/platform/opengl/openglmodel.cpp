#include "openglmodel.h"

#include <assimp/postprocess.h>

#include <assimp/Importer.hpp>
#include <cassert>
#include <vector>

#include "core/log.h"
#include "openglresourcemanager.h"

#define TINYOBJLOADER_IMPLEMENTATION
// earcut gives robust triangulation
// #define TINYOBJLOADER_USE_MAPBOX_EARCUT
#include "tiny_obj_loader.h"

void OpenGLModel::load(const std::string &path) {
    assert(!path.empty());

    meshes.clear();

#if USE_TINYOBJ
    auto baseDir = [](const std::string &filepath) {
        auto pos = filepath.find_last_of("/\\");
        if (pos != std::string::npos) {
            return filepath.substr(0, pos);
        }
        return std::string{};
    };

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;

    auto ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), baseDir(path).c_str());
    if (!warn.empty()) {
        SPONGE_CORE_WARN(warn);
    }

    if (!err.empty()) {
        SPONGE_CORE_ERROR(err);
    }

    if (!ret) {
        SPONGE_CORE_ERROR("Unable to load model: {}", path);
        return;
    }

    SPONGE_CORE_DEBUG("# of vertices   = {}", (int)(attrib.vertices.size()) / 3);
    SPONGE_CORE_DEBUG("# of normals    = {}", (int)(attrib.normals.size()) / 3);
    SPONGE_CORE_DEBUG("# of tex coords = {}", (int)(attrib.texcoords.size()) / 2);
    SPONGE_CORE_DEBUG("# of materials  = {}", (int)materials.size());
    SPONGE_CORE_DEBUG("# of shapes     = {}", (int)shapes.size());
#else
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(path, 0);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        SPONGE_CORE_ERROR("Unable to load model: {}", path);
        return;
    }

    importer.ApplyPostProcessing(aiProcess_GenNormals);

    processNode(scene->mRootNode, scene);
#endif
}

// TODO calculate normal
// normal = glm::normalize(glm::cross(p[i+2]-p[i], p[i+1] - p[i]))

#if !USE_TINYOBJ
void OpenGLModel::processNode(const aiNode *node, const aiScene *scene) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        const aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

OpenGLMesh OpenGLModel::processMesh(const aiMesh *mesh, const aiScene *scene) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex{};

        glm::vec3 glmVec3 = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
        vertex.position = glmVec3;

        if (mesh->HasNormals()) {
            glmVec3 = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
            vertex.normal = glmVec3;
        }

        if (mesh->mTextureCoords[0] != nullptr) {
            glm::vec2 glmVec2 = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
            vertex.texCoords = glmVec2;
        } else {
            vertex.texCoords = glm::vec2(0.0f, 0.0f);
        }

        if (mesh->mTangents != nullptr) {
            glmVec3 = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
            vertex.tangent = glmVec3;
        }

        if (mesh->mBitangents != nullptr) {
            glmVec3 = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
            vertex.biTangent = glmVec3;
        }

        vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // process materials
    const aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
    // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
    // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER.
    // Same applies to other texture as the following list summarizes:
    // diffuse: texture_diffuseN
    // specular: texture_specularN
    // normal: texture_normalN

    std::vector<std::shared_ptr<OpenGLTexture>> textures;

    // 1. diffuse maps
    auto diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    // 2. specular maps
    auto specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    // 3. normal maps
    auto normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
    // 4. height maps
    auto heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
    textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

    return { vertices, indices, textures };
}

std::vector<std::shared_ptr<OpenGLTexture>> OpenGLModel::loadMaterialTextures(const aiMaterial *mat,
                                                                              aiTextureType textureType,
                                                                              const std::string &typeName) {
    std::vector<std::shared_ptr<OpenGLTexture>> textures;

    for (unsigned int i = 0; i < mat->GetTextureCount(textureType); i++) {
        aiString str;
        mat->GetTexture(textureType, i, &str);

        std::string filename = std::string("assets/meshes/") + std::string(str.C_Str());
        auto texture = OpenGLResourceManager::loadTextureWithType(filename, typeName);

        textures.push_back(texture);
    }

    return textures;
}
#endif

void OpenGLModel::render() {
    for (auto it = std::begin(meshes); it != std::end(meshes); ++it) {
        it->render();
    }
}
