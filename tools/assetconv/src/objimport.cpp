#include "objimport.hpp"

#include "modeldata.hpp"
#include "tangents.hpp"

#include <fmt/base.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <stb_image.h>
#include <tiny_obj_loader.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace {
using sponge::scene::ModelData;
using sponge::scene::ParsedImage;
using sponge::scene::ParsedMesh;
using sponge::scene::Vertex;

std::string baseName(const std::string& filepath) {
    if (const auto pos = filepath.find_last_of("/\\");
        pos != std::string::npos) {
        return filepath.substr(pos + 1, filepath.length());
    }
    return filepath;
}

// Index into ModelData::images by image name, so a texture shared by several
// materials decodes once. Empty when the decode failed.
using ImageIndex = std::map<std::string, std::optional<uint32_t>>;

// OBJ materials only carry a diffuse map; the rest of the PBR slots stay
// empty and the engine falls back to its material factors.
std::optional<ParsedImage>
    decodeMaterialTexture(const tinyobj::material_t& material,
                          const std::string& path, const std::string& name) {
    const auto filename = std::filesystem::weakly_canonical(
        std::filesystem::path(path) / baseName(material.diffuse_texname));

    int   width         = 0;
    int   height        = 0;
    int   bytesPerPixel = 0;
    auto* pixels =
        stbi_load(filename.string().data(), &width, &height, &bytesPerPixel, 0);
    if (pixels == nullptr) {
        fmt::println(stderr, "assetconv: unable to decode {}: {}",
                     filename.string(), stbi_failure_reason());
        return std::nullopt;
    }

    ParsedImage image{
        .name          = name,
        .width         = static_cast<uint32_t>(width),
        .height        = static_cast<uint32_t>(height),
        .bytesPerPixel = static_cast<uint32_t>(bytesPerPixel),
        .pixels = { pixels, pixels + (static_cast<size_t>(width) * height *
                                      bytesPerPixel) },
        .ktx2   = {},
    };
    stbi_image_free(pixels);
    return image;
}

std::optional<uint32_t> materialTexture(const tinyobj::material_t& material,
                                        const std::string&         path,
                                        ModelData&                 data,
                                        ImageIndex&                imageIndex) {
    auto name = baseName(material.diffuse_texname);
    std::ranges::transform(name, name.begin(),
                           [](const uint8_t c) { return std::tolower(c); });

    auto [entry, inserted] = imageIndex.try_emplace(name);
    if (inserted) {
        if (auto decoded = decodeMaterialTexture(material, path, name)) {
            entry->second = static_cast<uint32_t>(data.images.size());
            data.images.push_back(std::move(*decoded));
        }
    }
    return entry->second;
}

ParsedMesh parseMesh(const tinyobj::attrib_t&                attrib,
                     const tinyobj::mesh_t&                  mesh,
                     const std::vector<tinyobj::material_t>& materials,
                     const std::string& path, ModelData& data,
                     ImageIndex& imageIndex) {
    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;

    vertices.reserve(mesh.indices.size());
    indices.reserve(mesh.indices.size());

    uint32_t nextIndex = 0;
    Vertex   vertex{};
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
            i = normalIndex * 3;
            // OBJ does not require unit normals; tangent generation does.
            vertex.normal =
                normalize(glm::vec3{ attrib.normals[i], attrib.normals[i + 1],
                                     attrib.normals[i + 2] });
        }

        vertices.emplace_back(vertex);
        indices.emplace_back(nextIndex);
        nextIndex++;
    }

    // calculate normals since they are missing
    if (attrib.normals.empty()) {
        for (size_t j = 0; j + 2 < vertices.size(); j += 3) {
            const auto p0 = vertices[j + 0].position;
            const auto p1 = vertices[j + 1].position;
            const auto p2 = vertices[j + 2].position;

            const auto normal = normalize(cross(p1 - p0, p2 - p0));

            vertices[j + 0].normal = normal;
            vertices[j + 1].normal = normal;
            vertices[j + 2].normal = normal;
        }
    }

    // The engine never sees the source format, so an OBJ mesh has to arrive
    // with the same vertex data a glTF one does.
    assetconv::computeTangents(vertices, indices);

    ParsedMesh parsedMesh;
    if (!mesh.material_ids.empty()) {
        if (const auto id = mesh.material_ids[0];
            id != -1 && !materials[id].diffuse_texname.empty()) {
            parsedMesh.albedo =
                materialTexture(materials[id], path, data, imageIndex);
        }
    }

    parsedMesh.vertices = std::move(vertices);
    parsedMesh.indices  = std::move(indices);
    return parsedMesh;
}
}  // namespace

namespace assetconv::obj {

sponge::scene::ModelData parse(const std::string& path) {
    ModelData  data;
    ImageIndex imageIndex;

    tinyobj::attrib_t                attrib;
    std::vector<tinyobj::shape_t>    shapes;
    std::vector<tinyobj::material_t> materials;
    std::string                      warn;
    std::string                      err;

    const std::filesystem::path dir{ path };
    const auto                  parentPath = dir.parent_path().string();
    const auto ret = LoadObj(&attrib, &shapes, &materials, &warn, &err,
                             dir.string().data(), parentPath.data());

    if (!warn.empty()) {
        fmt::println(stderr, "assetconv: {}", warn);
    }
    if (!err.empty()) {
        fmt::println(stderr, "assetconv: {}", err);
    }
    if (!ret) {
        fmt::println(stderr, "assetconv: unable to load {}", dir.string());
        return data;
    }

    for (const auto& shape : shapes) {
        auto mesh = parseMesh(attrib, shape.mesh, materials, parentPath, data,
                              imageIndex);
        data.meshes.emplace_back(std::move(mesh));
    }

    return data;
}

}  // namespace assetconv::obj
