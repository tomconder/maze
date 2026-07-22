#include "platform/opengl/scene/model.hpp"

#include "core/timer.hpp"
#include "debug/profiler.hpp"
#include "logging/log.hpp"
#include "platform/opengl/debug/profiler.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <filesystem>
#include <memory>
#include <ranges>
#include <string>
#include <vector>

#define TINYOBJLOADER_IMPLEMENTATION
// earcut gives robust triangulation
// #define TINYOBJLOADER_USE_MAPBOX_EARCUT
#include "tiny_obj_loader.h"

#include <cgltf.h>
#include <stb_image.h>

namespace {
constexpr double secondsToMilliseconds = 1000.F;
}

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;
using sponge::scene::Vertex;

Model::Model(const ModelCreateInfo& createInfo) {
    assert(!createInfo.path.empty());
    SPONGE_GL_INFO("Loading model file: [{}, {}]", createInfo.name,
                   createInfo.path);
    load(createInfo.assetsFolder + createInfo.path);
}

void Model::load(const std::string& path) {
    assert(!path.empty());

    const auto extension = std::filesystem::path(path).extension().string();
    if (extension == ".glb" || extension == ".gltf") {
        loadGltf(path);
    } else {
        loadObj(path);
    }
}

void Model::loadObj(const std::string& path) {
    meshes.clear();

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
        return;
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

    process(attrib, shapes, materials, dir.parent_path().string());
}

void Model::process(tinyobj::attrib_t&                      attrib,
                    std::vector<tinyobj::shape_t>&          shapes,
                    const std::vector<tinyobj::material_t>& materials,
                    const std::string&                      path) {
    numIndices  = 0;
    numVertices = 0;

    for (auto& [name, mesh, lines, points] : shapes) {
        auto newMesh = processMesh(attrib, mesh, materials, path);
        newMesh->optimize();
        numIndices += newMesh->getNumIndices();
        numVertices += newMesh->getNumVertices();
        meshes.emplace_back(newMesh);
    }
}

std::shared_ptr<Mesh>
    Model::processMesh(tinyobj::attrib_t& attrib, tinyobj::mesh_t& mesh,
                       const std::vector<tinyobj::material_t>& materials,
                       const std::string&                      path) {
    std::vector<Vertex>                             vertices;
    std::vector<uint32_t>                           indices;
    std::vector<std::shared_ptr<renderer::Texture>> textures;

    auto numIndices = 0;

    vertices.reserve(mesh.indices.size());
    indices.reserve(mesh.indices.size());

    Vertex vertex{};
    for (auto [vertex_index, normal_index, texcoord_index] : mesh.indices) {
        auto i          = vertex_index * 3;
        vertex.position = glm::vec3{ attrib.vertices[i], attrib.vertices[i + 1],
                                     attrib.vertices[i + 2] };

        if (!attrib.texcoords.empty()) {
            i                = texcoord_index * 2;
            vertex.texCoords = glm::vec2{ attrib.texcoords[i],
                                          1.0F - attrib.texcoords[i + 1] };
        } else {
            vertex.texCoords = glm::zero<glm::vec2>();
        }

        if (!attrib.normals.empty()) {
            i             = normal_index * 3;
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

    return std::make_shared<Mesh>(std::move(vertices), numIndices,
                                  std::move(indices), numIndices,
                                  std::move(textures));
}

std::shared_ptr<renderer::Texture>
    Model::loadMaterialTextures(const tinyobj::material_t& material,
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
    std::transform(name.begin(), name.end(), name.begin(),
                   [](const uint8_t c) { return std::tolower(c); });

    const renderer::TextureCreateInfo textureCreateInfo{
        .name     = name,
        .path     = filename.string(),
        .loadFlag = renderer::ExcludeAssetsFolder
    };
    return AssetManager::createTexture(textureCreateInfo);
}

void Model::loadGltf(const std::string& path) {
    meshes.clear();

    core::Timer timer;
    timer.tick();

    cgltf_options options{};
    cgltf_data*   data = nullptr;
    if (cgltf_parse_file(&options, path.c_str(), &data) !=
        cgltf_result_success) {
        SPONGE_GL_ERROR("Unable to parse gltf model: {}", path);
        return;
    }

    if (cgltf_load_buffers(&options, data, path.c_str()) !=
        cgltf_result_success) {
        SPONGE_GL_ERROR("Unable to load gltf buffers: {}", path);
        cgltf_free(data);
        return;
    }

    numIndices  = 0;
    numVertices = 0;

    // Bake each node's world transform into its mesh's vertices so glTF
    // files authored Z-up (or otherwise offset) render the same as their
    // node hierarchy intends, instead of in raw local mesh space.
    for (size_t n = 0; n < data->nodes_count; n++) {
        const auto& node = data->nodes[n];
        if (node.mesh == nullptr) {
            continue;
        }

        std::array<float, 16> worldMatrix{};
        cgltf_node_transform_world(&node, worldMatrix.data());
        const auto transform = glm::make_mat4(worldMatrix.data());

        for (size_t p = 0; p < node.mesh->primitives_count; p++) {
            auto newMesh =
                processGltfPrimitive(node.mesh->primitives[p], transform, path);
            if (!newMesh) {
                continue;
            }
            newMesh->optimize();
            numIndices += newMesh->getNumIndices();
            numVertices += newMesh->getNumVertices();
            meshes.emplace_back(newMesh);
        }
    }

    cgltf_free(data);

    timer.tick();
    SPONGE_GL_DEBUG("Parsing time for model: {:.2f} ms",
                    timer.getElapsedSeconds() * secondsToMilliseconds);
    SPONGE_GL_DEBUG("# of vertices  = {}", static_cast<int>(numVertices));
    SPONGE_GL_DEBUG("# of meshes    = {}", static_cast<int>(meshes.size()));
}

std::shared_ptr<Mesh>
    Model::processGltfPrimitive(const cgltf_primitive& primitive,
                                const glm::mat4&       transform,
                                const std::string&     path) {
    if (primitive.type != cgltf_primitive_type_triangles) {
        return nullptr;
    }

    const auto normalMatrix = glm::mat3(transpose(inverse(transform)));

    const cgltf_accessor* positionAccessor = nullptr;
    const cgltf_accessor* normalAccessor   = nullptr;
    const cgltf_accessor* texcoordAccessor = nullptr;

    for (size_t a = 0; a < primitive.attributes_count; a++) {
        const auto& attribute = primitive.attributes[a];
        if (attribute.type == cgltf_attribute_type_position) {
            positionAccessor = attribute.data;
        } else if (attribute.type == cgltf_attribute_type_normal) {
            normalAccessor = attribute.data;
        } else if (attribute.type == cgltf_attribute_type_texcoord &&
                   attribute.index == 0) {
            texcoordAccessor = attribute.data;
        }
    }

    if (positionAccessor == nullptr) {
        return nullptr;
    }

    const auto vertexCount = positionAccessor->count;

    std::vector<Vertex> vertices(vertexCount);
    for (cgltf_size i = 0; i < vertexCount; i++) {
        glm::vec3 position(0.F);
        cgltf_accessor_read_float(positionAccessor, i, &position.x, 3);
        vertices[i].position = glm::vec3(transform * glm::vec4(position, 1.F));

        if (texcoordAccessor != nullptr) {
            glm::vec2 texCoords(0.F);
            cgltf_accessor_read_float(texcoordAccessor, i, &texCoords.x, 2);
            vertices[i].texCoords = texCoords;
        }

        if (normalAccessor != nullptr) {
            glm::vec3 normal(0.F);
            cgltf_accessor_read_float(normalAccessor, i, &normal.x, 3);
            normal             = normalize(normalMatrix * normal);
            vertices[i].normal = normal;
        }
    }

    std::vector<uint32_t> indices;
    if (primitive.indices != nullptr) {
        indices.resize(primitive.indices->count);
        for (cgltf_size i = 0; i < primitive.indices->count; i++) {
            cgltf_uint index = 0;
            cgltf_accessor_read_uint(primitive.indices, i, &index, 1);
            indices[i] = index;
        }
    } else {
        indices.resize(vertexCount);
        for (cgltf_size i = 0; i < vertexCount; i++) {
            indices[i] = static_cast<uint32_t>(i);
        }
    }

    // calculate normals since they are missing
    if (normalAccessor == nullptr) {
        for (size_t i = 0; i + 2 < indices.size(); i += 3) {
            auto& v0 = vertices[indices[i]];
            auto& v1 = vertices[indices[i + 1]];
            auto& v2 = vertices[indices[i + 2]];

            const auto normal = normalize(
                cross(v1.position - v0.position, v2.position - v0.position));
            v0.normal = normal;
            v1.normal = normal;
            v2.normal = normal;
        }
    }

    computeTangents(vertices, indices);

    std::vector<std::shared_ptr<renderer::Texture>> textures;
    std::shared_ptr<renderer::Texture>              normalTexture;
    std::shared_ptr<renderer::Texture>              occlusionTexture;
    std::shared_ptr<renderer::Texture>              emissiveTexture;
    std::shared_ptr<renderer::Texture>              metallicRoughnessTexture;
    // Defaults match the non-PBR-textured (obj) baseline: mostly rough
    // dielectric.
    float            metallicFactor  = 0.F;
    float            roughnessFactor = .5F;
    MeshUVTransforms uvTransforms;
    if (primitive.material != nullptr) {
        const auto& material = *primitive.material;
        if (material.has_pbr_metallic_roughness) {
            const auto& pbr = material.pbr_metallic_roughness;
            if (auto texture = loadGltfTexture(pbr.base_color_texture, path)) {
                textures.emplace_back(std::move(texture));
            }
            metallicRoughnessTexture =
                loadGltfTexture(pbr.metallic_roughness_texture, path);
            metallicFactor      = pbr.metallic_factor;
            roughnessFactor     = pbr.roughness_factor;
            uvTransforms.albedo = gltfUVTransform(pbr.base_color_texture);
            uvTransforms.metallicRoughness =
                gltfUVTransform(pbr.metallic_roughness_texture);
        }
        normalTexture       = loadGltfTexture(material.normal_texture, path);
        occlusionTexture    = loadGltfTexture(material.occlusion_texture, path);
        emissiveTexture     = loadGltfTexture(material.emissive_texture, path);
        uvTransforms.normal = gltfUVTransform(material.normal_texture);
        uvTransforms.occlusion = gltfUVTransform(material.occlusion_texture);
        uvTransforms.emissive  = gltfUVTransform(material.emissive_texture);
    }

    const auto indexCount = indices.size();
    return std::make_shared<Mesh>(
        std::move(vertices), vertexCount, std::move(indices), indexCount,
        std::move(textures), std::move(normalTexture),
        std::move(occlusionTexture), std::move(emissiveTexture),
        std::move(metallicRoughnessTexture), metallicFactor, roughnessFactor,
        uvTransforms);
}

UVTransform Model::gltfUVTransform(const cgltf_texture_view& textureView) {
    if (!textureView.has_transform) {
        return {};
    }
    const auto& t = textureView.transform;
    if (t.rotation != 0.F) {
        SPONGE_GL_WARN(
            "KHR_texture_transform rotation is not supported; ignoring");
    }
    return UVTransform{ .offset = glm::vec2(t.offset[0], t.offset[1]),
                        .scale  = glm::vec2(t.scale[0], t.scale[1]) };
}

// Per-triangle tangent accumulation (Lengyel's method), averaged per vertex
// and Gram-Schmidt orthogonalized against the vertex normal.
void Model::computeTangents(std::vector<Vertex>&         vertices,
                            const std::vector<uint32_t>& indices) {
    std::vector<glm::vec3> tan(vertices.size(), glm::vec3(0.F));
    std::vector<glm::vec3> bitan(vertices.size(), glm::vec3(0.F));

    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        const auto i0 = indices[i];
        const auto i1 = indices[i + 1];
        const auto i2 = indices[i + 2];

        const auto& v0 = vertices[i0];
        const auto& v1 = vertices[i1];
        const auto& v2 = vertices[i2];

        const auto edge1 = v1.position - v0.position;
        const auto edge2 = v2.position - v0.position;
        const auto duv1  = v1.texCoords - v0.texCoords;
        const auto duv2  = v2.texCoords - v0.texCoords;

        const auto det = duv1.x * duv2.y - duv2.x * duv1.y;
        if (glm::abs(det) < 1e-8F) {
            continue;
        }
        const auto f = 1.F / det;

        const auto tangent   = f * (duv2.y * edge1 - duv1.y * edge2);
        const auto bitangent = f * (duv1.x * edge2 - duv2.x * edge1);

        for (const auto idx : { i0, i1, i2 }) {
            tan[idx] += tangent;
            bitan[idx] += bitangent;
        }
    }

    for (size_t i = 0; i < vertices.size(); i++) {
        const auto& n = vertices[i].normal;
        auto        t = tan[i] - n * dot(n, tan[i]);
        if (dot(t, t) < 1e-12F) {
            // degenerate UVs; fall back to any vector orthogonal to normal
            t = glm::abs(n.x) > glm::abs(n.z) ? glm::vec3(-n.y, n.x, 0.F) :
                                                glm::vec3(0.F, -n.z, n.y);
        }
        t                   = normalize(t);
        const auto sign     = dot(cross(n, t), bitan[i]) < 0.F ? -1.F : 1.F;
        vertices[i].tangent = glm::vec4(t, sign);
    }
}

std::shared_ptr<renderer::Texture>
    Model::loadGltfTexture(const cgltf_texture_view& textureView,
                           const std::string&        path) {
    const auto* texture = textureView.texture;
    if (texture == nullptr || texture->image == nullptr) {
        return nullptr;
    }

    const auto* image = texture->image;
    if (image->buffer_view == nullptr) {
        SPONGE_GL_WARN("Unsupported gltf image source (expected buffer view)");
        return nullptr;
    }

    const auto* bytes = cgltf_buffer_view_data(image->buffer_view);
    const auto  size  = static_cast<int>(image->buffer_view->size);

    // Cache key: file path + byte range within it. Not the cgltf_image*
    // address, which is only valid until cgltf_free() and gets reused by
    // later, unrelated model loads, causing cache collisions (one model
    // silently sampling another model's texture).
    const auto  name = path + "#" + std::to_string(image->buffer_view->offset) +
                       "_" + std::to_string(image->buffer_view->size);
    const auto& cachedTextures = AssetManager::getTextures();
    if (const auto it = cachedTextures.find(name); it != cachedTextures.end()) {
        return it->second;
    }

    int   width         = 0;
    int   height        = 0;
    int   bytesPerPixel = 0;
    auto* pixels =
        stbi_load_from_memory(bytes, size, &width, &height, &bytesPerPixel, 0);
    if (pixels == nullptr) {
        SPONGE_GL_ERROR("Unable to decode gltf image: {}",
                        stbi_failure_reason());
        return nullptr;
    }

    const renderer::TextureCreateInfo textureCreateInfo{
        .name          = name,
        .path          = "",
        .width         = static_cast<uint32_t>(width),
        .height        = static_cast<uint32_t>(height),
        .bytesPerPixel = static_cast<uint32_t>(bytesPerPixel),
        .data          = pixels,
    };
    auto result = AssetManager::createTexture(textureCreateInfo);
    stbi_image_free(pixels);
    return result;
}

void Model::render(const std::shared_ptr<renderer::Shader>& shader) const {
    SPONGE_PROFILE;
    SPONGE_PROFILE_GPU("render model");

    for (auto&& mesh : meshes) {
        mesh->render(shader);
    }
}
}  // namespace sponge::platform::opengl::scene
