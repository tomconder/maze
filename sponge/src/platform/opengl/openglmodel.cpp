#include "platform/opengl/openglmodel.h"
#include "core/file.h"
#include "platform/opengl/openglresourcemanager.h"
#include <cassert>

#define TINYOBJLOADER_IMPLEMENTATION
// earcut gives robust triangulation
// #define TINYOBJLOADER_USE_MAPBOX_EARCUT
#include "core/log.h"
#include "tiny_obj_loader.h"

namespace sponge::renderer {

void OpenGLModel::load(std::string_view path) {
    assert(!path.empty());

    meshes.clear();

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;

    std::filesystem::path dir{ path.data() };
    auto ret = LoadObj(&attrib, &shapes, &materials, &warn, &err,
                       dir.string().data(), dir.parent_path().string().data());

    if (!warn.empty()) {
        SPONGE_CORE_WARN(warn);
    }

    if (!err.empty()) {
        SPONGE_CORE_ERROR(err);
    }

    if (!ret) {
        SPONGE_CORE_ERROR("Unable to load model: {}", dir.string());
        return;
    }

    SPONGE_CORE_INFO("# of vertices  = {}",
                     static_cast<int>(attrib.vertices.size() / 3));
    SPONGE_CORE_INFO("# of normals   = {}",
                     static_cast<int>(attrib.normals.size() / 3));
    SPONGE_CORE_INFO("# of texcoords = {}",
                     static_cast<int>(attrib.texcoords.size() / 2));
    SPONGE_CORE_INFO("# of materials = {}", static_cast<int>(materials.size()));
    SPONGE_CORE_INFO("# of shapes    = {}", static_cast<int>(shapes.size()));

    process(attrib, shapes, materials);
}

void OpenGLModel::process(tinyobj::attrib_t& attrib,
                          std::vector<tinyobj::shape_t>& shapes,
                          const std::vector<tinyobj::material_t>& materials) {
    numIndices = 0;
    numVertices = 0;

    for (auto& shape : shapes) {
        auto mesh = processMesh(attrib, shape.mesh, materials);
        mesh->optimize();
        numIndices += mesh->getNumIndices();
        numVertices += mesh->getNumVertices();
        meshes.push_back(mesh);
    }
}

std::shared_ptr<OpenGLMesh> OpenGLModel::processMesh(
    tinyobj::attrib_t& attrib, tinyobj::mesh_t& mesh,
    const std::vector<tinyobj::material_t>& materials) {
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
        vertex.position =
            glm::vec3{ attrib.vertices[i * 3 + 0], attrib.vertices[i * 3 + 1],
                       attrib.vertices[i * 3 + 2] };

        i = index.texcoord_index;
        if (i >= 0) {
            vertex.texCoords = glm::vec2{ attrib.texcoords[i * 2 + 0],
                                          1.0F - attrib.texcoords[i * 2 + 1] };
        }

        i = index.normal_index;
        if (i >= 0) {
            vertex.normal =
                glm::vec3{ attrib.normals[i * 3 + 0], attrib.normals[i * 3 + 1],
                           attrib.normals[i * 3 + 2] };
        }

        vertices.push_back(vertex);
        indices.push_back(numIndices);
        numIndices++;
    }

    // recalculate normals since they may be missing
    for (auto j = 0; j < vertices.size(); j += 3) {
        const auto p0 = vertices[j + 0].position;
        const auto p1 = vertices[j + 1].position;
        const auto p2 = vertices[j + 2].position;

        const auto normal = normalize(cross(p1 - p0, p2 - p0));

        vertices[j + 0].normal = normal;
        vertices[j + 1].normal = normal;
        vertices[j + 2].normal = normal;
    }

    if (!mesh.material_ids.empty()) {
        const auto id = mesh.material_ids[0];
        if (id != -1) {
            textures.push_back(loadMaterialTextures(materials[id]));
        }
    }

    return std::make_shared<OpenGLMesh>(vertices, indices, textures);
}

std::shared_ptr<OpenGLTexture> OpenGLModel::loadMaterialTextures(
    const tinyobj::material_t& material) {
    std::shared_ptr<OpenGLTexture> texture;

    auto baseName = [](const std::string& filepath) {
        if (auto pos = filepath.find_last_of("/\\"); pos != std::string::npos) {
            return filepath.substr(pos + 1, filepath.length());
        }
        return filepath;
    };

    auto assetsFolder = File::getResourceDir();
    auto filename = std::filesystem::weakly_canonical(
        assetsFolder + "/models/" + baseName(material.diffuse_texname));

    auto name = baseName(material.diffuse_texname);
    std::transform(name.begin(), name.end(), name.begin(),
                   [](uint8_t c) { return std::tolower(c); });

    return OpenGLResourceManager::loadTexture(filename.string(), name);
}

void OpenGLModel::render() const {
    for (auto&& mesh : meshes) {
        mesh->render();
    }
}

}  // namespace sponge::renderer
