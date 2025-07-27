#include "model.hpp"
#include "core/timer.hpp"
#include "debug/profiler.hpp"
#include "logging/log.hpp"
#include "platform/opengl/debug/profiler.hpp"
#include "platform/opengl/renderer/resourcemanager.hpp"
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <ranges>

#define TINYOBJLOADER_IMPLEMENTATION
// earcut gives robust triangulation
// #define TINYOBJLOADER_USE_MAPBOX_EARCUT
#include "tiny_obj_loader.h"

namespace {
constexpr double secondsToMilliseconds = 1000.F;
}

namespace sponge::platform::opengl::scene {
using sponge::scene::Vertex;

Model::Model(const ModelCreateInfo& createInfo) {
    assert(!createInfo.path.empty());
    load(createInfo.assetsFolder + createInfo.path);
}

void Model::load(const std::string& path) {
    assert(!path.empty());

    SPONGE_CORE_INFO("Loading model file: [{}]", path);

    meshes.clear();

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;

    core::Timer timer;
    timer.tick();

    const std::filesystem::path dir{ path };
    const auto ret =
        LoadObj(&attrib, &shapes, &materials, &warn, &err, dir.string().data(),
                dir.parent_path().string().data());

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

    timer.tick();

    // from viewer.cc in tinyobjloader example
    SPONGE_CORE_DEBUG("Parsing time: {:.2f} ms",
                      timer.getElapsedSeconds() * secondsToMilliseconds);

    SPONGE_CORE_DEBUG("# of vertices  = {}",
                      static_cast<int>(attrib.vertices.size() / 3));
    SPONGE_CORE_DEBUG("# of normals   = {}",
                      static_cast<int>(attrib.normals.size() / 3));
    SPONGE_CORE_DEBUG("# of texcoords = {}",
                      static_cast<int>(attrib.texcoords.size() / 2));
    SPONGE_CORE_DEBUG("# of materials = {}",
                      static_cast<int>(materials.size()));
    SPONGE_CORE_DEBUG("# of shapes    = {}", static_cast<int>(shapes.size()));

    process(attrib, shapes, materials, dir.parent_path().string());
}

void Model::process(tinyobj::attrib_t& attrib,
                    std::vector<tinyobj::shape_t>& shapes,
                    const std::vector<tinyobj::material_t>& materials,
                    const std::string& path) {
    numIndices = 0;
    numVertices = 0;

    for (auto& [name, mesh, lines, points] : shapes) {
        auto newMesh = processMesh(attrib, mesh, materials, path);
        newMesh->optimize();
        numIndices += newMesh->getNumIndices();
        numVertices += newMesh->getNumVertices();
        meshes.emplace_back(newMesh);
    }
}

std::shared_ptr<Mesh> Model::processMesh(
    tinyobj::attrib_t& attrib, tinyobj::mesh_t& mesh,
    const std::vector<tinyobj::material_t>& materials,
    const std::string& path) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<std::shared_ptr<renderer::Texture>> textures;

    auto numIndices = 0;

    vertices.reserve(mesh.indices.size());
    indices.reserve(mesh.indices.size());

    Vertex vertex{};
    for (auto [vertex_index, normal_index, texcoord_index] : mesh.indices) {
        auto i = vertex_index * 3;
        vertex.position = glm::vec3{ attrib.vertices[i], attrib.vertices[i + 1],
                                     attrib.vertices[i + 2] };

        if (!attrib.texcoords.empty()) {
            i = texcoord_index * 2;
            vertex.texCoords = glm::vec2{ attrib.texcoords[i],
                                          1.0F - attrib.texcoords[i + 1] };
        } else {
            vertex.texCoords = glm::zero<glm::vec2>();
        }

        if (!attrib.normals.empty()) {
            i = normal_index * 3;
            vertex.normal = glm::vec3{ attrib.normals[i], attrib.normals[i + 1],
                                       attrib.normals[i + 2] };
        }

        vertices.emplace_back(vertex);
        indices.emplace_back(numIndices);
        numIndices++;
    }

    // calculate normals since they are missing
    if (attrib.normals.empty()) {
        for (auto j = 0; j < vertices.size(); j += 3) {
            const auto p0 = vertices[j + 0].position;
            const auto p1 = vertices[j + 1].position;
            const auto p2 = vertices[j + 2].position;

            const auto normal = normalize(cross(p1 - p0, p2 - p0));

            vertices[j + 0].normal = normal;
            vertices[j + 1].normal = normal;
            vertices[j + 2].normal = normal;
        }
    }

    if (!mesh.material_ids.empty()) {
        if (const auto id = mesh.material_ids[0]; id != -1) {
            if (!materials[id].diffuse_texname.empty()) {
                textures.emplace_back(
                    loadMaterialTextures(materials[id], path));
            }
        }
    }

    return std::make_shared<Mesh>(vertices, numIndices, indices, numIndices,
                                  textures);
}

std::shared_ptr<renderer::Texture> Model::loadMaterialTextures(
    const tinyobj::material_t& material, const std::string& path) {
    auto baseName = [](const std::string& filepath) {
        if (const auto pos = filepath.find_last_of("/\\");
            pos != std::string::npos) {
            return filepath.substr(pos + 1, filepath.length());
        }
        return filepath;
    };

    const auto filename = std::filesystem::weakly_canonical(
        path + "/" + baseName(material.diffuse_texname));

    auto name = baseName(material.diffuse_texname);
    std::ranges::transform(name, name.begin(),
                           [](const uint8_t c) { return std::tolower(c); });

    const renderer::TextureCreateInfo textureCreateInfo{
        .name = name,
        .path = filename.string(),
        .loadFlag = renderer::ExcludeAssetsFolder
    };
    return renderer::ResourceManager::loadTexture(textureCreateInfo);
}

void Model::render(std::shared_ptr<renderer::Shader>& shader) const {
    SPONGE_PROFILE;
    SPONGE_PROFILE_GPU("render model");

    for (auto&& mesh : meshes) {
        mesh->render(shader);
    }
}
}  // namespace sponge::platform::opengl::scene
