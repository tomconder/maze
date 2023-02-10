#include "openglmodel.h"

#include <cassert>
#include <vector>

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
}

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
                                          1.0f - attrib.texcoords[i * 2 + 1] };
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

    if (mesh.material_ids.size() > 0) {
        auto id = mesh.material_ids[0];
        if (id != -1) {
            textures.push_back(loadMaterialTextures(materials[id]));
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

    auto name = material.diffuse_texname;

    std::string filename = std::string("assets/models/") + baseName(name);
    return OpenGLResourceManager::loadTexture(filename, name);
}

void OpenGLModel::render() {
    for (auto it = std::begin(meshes); it != std::end(meshes); ++it) {
        it->render();
    }
}
