#include "openglmodel.h"

#include <assimp/postprocess.h>

#include <assimp/Importer.hpp>
#include <cassert>
#include <vector>

#include "core/log.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "openglresourcemanager.h"

#define TINYOBJLOADER_IMPLEMENTATION
// earcut gives robust triangulation
#define TINYOBJLOADER_USE_MAPBOX_EARCUT
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

    process(attrib, shapes, materials);
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

#if USE_TINYOBJ
void OpenGLModel::process(tinyobj::attrib_t &attrib, std::vector<tinyobj::shape_t> &shapes,
                          const std::vector<tinyobj::material_t> &materials) {
    for (auto &shape : shapes) {
        meshes.push_back(processMesh(attrib, shape.mesh, materials));
    }
}

OpenGLMesh OpenGLModel::processMesh(tinyobj::attrib_t &attrib, tinyobj::mesh_t &mesh,
                                    const std::vector<tinyobj::material_t> &materials) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<std::shared_ptr<OpenGLTexture>> textures;

    // vertices
    auto numIndices = 0;
    vertices.reserve(mesh.indices.size());
    indices.reserve(mesh.indices.size());
    for (auto index : mesh.indices) {
        Vertex vertex{};

        auto i = index.vertex_index;
        vertex.position = glm::vec3{ attrib.vertices[i * 3 + 0],  //
                                     attrib.vertices[i * 3 + 1],  //
                                     attrib.vertices[i * 3 + 2] };

        i = index.texcoord_index;
        if (i >= 0) {
            vertex.texCoords = glm::vec2{ attrib.texcoords[i * 2 + 0],  //
                                          attrib.texcoords[i * 2 + 1] };
        }

        i = index.normal_index;
        if (i >= 0) {
            vertex.normal = glm::vec3{ attrib.normals[i * 3 + 0],  //
                                       attrib.normals[i * 3 + 1],  //
                                       attrib.normals[i * 3 + 2] };
        }

        vertices.push_back(vertex);
        indices.push_back(numIndices++);
    }

    // recalculate normals since they may be missing
    for (auto j = 0; j < vertices.size(); j += 3) {
        auto p0 = vertices[j + 0].position;
        auto p1 = vertices[j + 1].position;
        auto p2 = vertices[j + 2].position;

        auto normal = glm::normalize(glm::cross(p1 - p0, p2 - p0));

        vertices[j + 0].normal = normal;
        vertices[j + 1].normal = normal;
        vertices[j + 2].normal = normal;
    }

    // process materials
    for (auto id : mesh.material_ids) {
        if (id >= 0) {
            const auto &material = materials[id];
            textures.push_back(loadMaterialTextures(material));
        }
    }

    return { vertices, indices, textures };
}

std::shared_ptr<OpenGLTexture> OpenGLModel::loadMaterialTextures(const tinyobj::material_t &material) {
    std::shared_ptr<OpenGLTexture> texture;

    auto baseName = [](const std::string &filepath) {
        auto pos = filepath.find_last_of("/\\");
        if (pos != std::string::npos) {
            return filepath.substr(pos + 1, filepath.length());
        }
        return filepath;
    };

    auto name = baseName(material.diffuse_texname);

    std::string filename = std::string("assets/meshes/") + name;
    return OpenGLResourceManager::loadTexture(filename, name);
}
#endif

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
    //    auto specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
    //    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    // 3. normal maps
    //    auto normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
    //    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
    // 4. height maps
    //    auto heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
    //    textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

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
