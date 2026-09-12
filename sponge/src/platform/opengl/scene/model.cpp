#include "platform/opengl/scene/model.hpp"

#include "core/timer.hpp"
#include "debug/profiler.hpp"
#include "logging/log.hpp"
#include "platform/opengl/debug/profiler.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"
#include "scene/assetformat.hpp"
#include "scene/gltfimport.hpp"

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <vector>

#include "tiny_obj_loader.h"

#include <glm/glm.hpp>
#include <stb_image.h>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace {
[[maybe_unused]] constexpr double secondsToMilliseconds = 1000.F;

std::vector<uint8_t> copyPixels(const uint8_t* pixels, const int width,
                                const int height, const int bytesPerPixel) {
    const auto* begin = pixels;
    const auto* end =
        pixels + (static_cast<size_t>(width) * height * bytesPerPixel);
    return { begin, end };
}
}  // namespace

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;
using sponge::scene::Vertex;

Model::Model(const ModelCreateInfo& createInfo) {
    assert(!createInfo.path.empty());

    for (auto& parsedMesh : parse(createInfo).meshes) {
        auto mesh = buildMesh(std::move(parsedMesh));
        numIndices += mesh->getNumIndices();
        numVertices += mesh->getNumVertices();
        meshes.emplace_back(std::move(mesh));
    }
}

Model::Model(std::vector<std::shared_ptr<Mesh>>&& builtMeshes) {
    for (const auto& mesh : builtMeshes) {
        numIndices += mesh->getNumIndices();
        numVertices += mesh->getNumVertices();
    }
    meshes = std::move(builtMeshes);
}

ModelData Model::parse(const ModelCreateInfo&       createInfo,
                       const std::function<void()>& onMeshParsed) {
    assert(!createInfo.path.empty());
    SPONGE_GL_INFO("Loading model file: [{}, {}]", createInfo.name,
                   createInfo.path);

    const auto path      = createInfo.assetsFolder + createInfo.path;
    const auto extension = std::filesystem::path(path).extension().string();
    if (extension == sponge::scene::asset::extension) {
        return sponge::scene::asset::read(path);
    }
    if (extension == ".glb" || extension == ".gltf") {
        return sponge::scene::gltf::parse(path, onMeshParsed);
    }
    return parseObj(path, onMeshParsed);
}

std::size_t Model::countMeshes(const ModelCreateInfo& createInfo) {
    const auto path      = createInfo.assetsFolder + createInfo.path;
    const auto extension = std::filesystem::path(path).extension().string();
    if (extension == sponge::scene::asset::extension) {
        return sponge::scene::asset::readMeshCount(path);
    }
    if (extension == ".glb" || extension == ".gltf") {
        return sponge::scene::gltf::countMeshes(path);
    }
    return 1;
}

std::shared_ptr<Mesh> Model::buildMesh(ParsedMesh&& parsedMesh) {
    // captured before the moves below: arg evaluation order is unspecified
    const auto vertexCount = parsedMesh.vertices.size();
    const auto indexCount  = parsedMesh.indices.size();

    std::vector<std::shared_ptr<renderer::Texture>> textures;
    if (auto albedo = buildTexture(std::move(parsedMesh.albedo))) {
        textures.emplace_back(std::move(albedo));
    }

    auto mesh = std::make_shared<Mesh>(
        std::move(parsedMesh.vertices), vertexCount,
        std::move(parsedMesh.indices), indexCount, std::move(textures),
        buildTexture(std::move(parsedMesh.normal)),
        buildTexture(std::move(parsedMesh.occlusion)),
        buildTexture(std::move(parsedMesh.emissive)),
        buildTexture(std::move(parsedMesh.metallicRoughness)),
        parsedMesh.metallicFactor, parsedMesh.roughnessFactor,
        parsedMesh.uvTransforms);
    mesh->optimize();
    return mesh;
}

std::shared_ptr<renderer::Texture>
    Model::buildTexture(std::optional<ParsedImage>&& image) {
    if (!image) {
        return nullptr;
    }

    // A baked image carries a KTX2 file; the glTF importer hands over raw
    // pixels. Texture takes whichever is populated.
    const renderer::TextureCreateInfo textureCreateInfo{
        .name          = image->name,
        .path          = "",
        .width         = image->width,
        .height        = image->height,
        .bytesPerPixel = image->bytesPerPixel,
        .data          = image->pixels.empty() ? nullptr : image->pixels.data(),
        .ktx2          = image->ktx2,
    };
    return AssetManager::createTexture(textureCreateInfo);
}

ModelData Model::parseObj(const std::string&           path,
                          const std::function<void()>& onMeshParsed) {
    ModelData data;

    tinyobj::attrib_t                attrib;
    std::vector<tinyobj::shape_t>    shapes;
    std::vector<tinyobj::material_t> materials;
    std::string                      warn;
    std::string                      err;

    core::Timer timer;
    timer.tick();

    const std::filesystem::path dir{ path };
    const auto                  parentPath = dir.parent_path().string();
    const auto ret = LoadObj(&attrib, &shapes, &materials, &warn, &err,
                             dir.string().data(), parentPath.data());

    if (!warn.empty()) {
        SPONGE_GL_WARN(warn);
    }

    if (!err.empty()) {
        SPONGE_GL_ERROR(err);
    }

    if (!ret) {
        SPONGE_GL_ERROR("Unable to load model: {}", dir.string());
        return data;
    }

    timer.tick();

    // from viewer.cc in tinyobjloader example
    SPONGE_GL_DEBUG("Parsing time for model: {:.2f} ms",
                    timer.getElapsedSeconds() * secondsToMilliseconds);

    SPONGE_GL_DEBUG("# of vertices  = {}",
                    static_cast<int>(attrib.vertices.size() / 3));
    SPONGE_GL_DEBUG("# of normals   = {}",
                    static_cast<int>(attrib.normals.size() / 3));
    SPONGE_GL_DEBUG("# of texcoords = {}",
                    static_cast<int>(attrib.texcoords.size() / 2));
    SPONGE_GL_DEBUG("# of materials = {}", static_cast<int>(materials.size()));
    SPONGE_GL_DEBUG("# of shapes    = {}", static_cast<int>(shapes.size()));

    for (auto& [name, mesh, lines, points] : shapes) {
        SPONGE_GL_INFO("Loading mesh: [{}]", name);
        data.meshes.emplace_back(
            parseObjMesh(attrib, mesh, materials, dir.parent_path().string()));
        if (onMeshParsed) {
            onMeshParsed();
        }
    }

    return data;
}

ParsedMesh
    Model::parseObjMesh(tinyobj::attrib_t& attrib, tinyobj::mesh_t& mesh,
                        const std::vector<tinyobj::material_t>& materials,
                        const std::string&                      path) {
    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;

    auto numIndices = 0;

    vertices.reserve(mesh.indices.size());
    indices.reserve(mesh.indices.size());

    Vertex vertex{};
    for (auto [vertexIndex, normalIndex, texcoordIndex] : mesh.indices) {
        auto i          = vertexIndex * 3;
        vertex.position = glm::vec3{ attrib.vertices[i], attrib.vertices[i + 1],
                                     attrib.vertices[i + 2] };

        if (!attrib.texcoords.empty()) {
            i                = texcoordIndex * 2;
            vertex.texCoords = glm::vec2{ attrib.texcoords[i],
                                          1.0F - attrib.texcoords[i + 1] };
        } else {
            vertex.texCoords = glm::zero<glm::vec2>();
        }

        if (!attrib.normals.empty()) {
            i             = normalIndex * 3;
            vertex.normal = glm::vec3{ attrib.normals[i], attrib.normals[i + 1],
                                       attrib.normals[i + 2] };
        }

        vertices.emplace_back(vertex);
        indices.emplace_back(numIndices);
        numIndices++;
    }

    // calculate normals since they are missing
    if (attrib.normals.empty()) {
        for (size_t j = 0; j < vertices.size(); j += 3) {
            const auto p0 = vertices[j + 0].position;
            const auto p1 = vertices[j + 1].position;
            const auto p2 = vertices[j + 2].position;

            const auto normal = normalize(cross(p1 - p0, p2 - p0));

            vertices[j + 0].normal = normal;
            vertices[j + 1].normal = normal;
            vertices[j + 2].normal = normal;
        }
    }

    ParsedMesh parsedMesh;
    if (!mesh.material_ids.empty()) {
        if (const auto id = mesh.material_ids[0];
            id != -1 && !materials[id].diffuse_texname.empty()) {
            parsedMesh.albedo = decodeMaterialTexture(materials[id], path);
        }
    }

    parsedMesh.vertices = std::move(vertices);
    parsedMesh.indices  = std::move(indices);
    return parsedMesh;
}

std::optional<ParsedImage>
    Model::decodeMaterialTexture(const tinyobj::material_t& material,
                                 const std::string&         path) {
    auto baseName = [](const std::string& filepath) {
        if (const auto pos = filepath.find_last_of("/\\");
            pos != std::string::npos) {
            return filepath.substr(pos + 1, filepath.length());
        }
        return filepath;
    };

    const auto filename = std::filesystem::weakly_canonical(
        std::filesystem::path(path) / baseName(material.diffuse_texname));

    auto name = baseName(material.diffuse_texname);
    std::ranges::transform(name, name.begin(),
                           [](const uint8_t c) { return std::tolower(c); });

    SPONGE_GL_INFO("Loading texture: [{}, {}]", name, filename.string());

    int   width         = 0;
    int   height        = 0;
    int   bytesPerPixel = 0;
    auto* pixels =
        stbi_load(filename.string().data(), &width, &height, &bytesPerPixel, 0);
    if (pixels == nullptr) {
        SPONGE_GL_ERROR("Unable to decode material texture: {}: {}",
                        filename.string(), stbi_failure_reason());
        return std::nullopt;
    }

    ParsedImage image{
        .name          = name,
        .width         = static_cast<uint32_t>(width),
        .height        = static_cast<uint32_t>(height),
        .bytesPerPixel = static_cast<uint32_t>(bytesPerPixel),
        .pixels        = copyPixels(pixels, width, height, bytesPerPixel),
        .ktx2          = {},
    };
    stbi_image_free(pixels);
    return image;
}

void Model::render(const std::shared_ptr<renderer::Shader>& shader) const {
    SPONGE_PROFILE;
    SPONGE_PROFILE_GPU("render model");

    for (auto&& mesh : meshes) {
        mesh->render(shader);
    }
}
}  // namespace sponge::platform::opengl::scene
