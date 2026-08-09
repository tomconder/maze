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
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <vector>

#include "tiny_obj_loader.h"

#include <glm/glm.hpp>
#include <cgltf.h>
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
    if (extension == ".glb" || extension == ".gltf") {
        return parseGltf(path, onMeshParsed);
    }
    return parseObj(path, onMeshParsed);
}

std::size_t Model::countMeshes(const ModelCreateInfo& createInfo) {
    const auto path      = createInfo.assetsFolder + createInfo.path;
    const auto extension = std::filesystem::path(path).extension().string();
    if (extension == ".glb" || extension == ".gltf") {
        return countGltfMeshes(path);
    }
    return 1;
}

std::size_t Model::countGltfMeshes(const std::string& path) {
    const cgltf_options options{};
    cgltf_data*         data = nullptr;
    if (cgltf_parse_file(&options, path.c_str(), &data) !=
        cgltf_result_success) {
        return 0;
    }

    std::size_t count = 0;
    for (size_t n = 0; n < data->nodes_count; n++) {
        const auto& node = data->nodes[n];
        if (node.mesh == nullptr) {
            continue;
        }
        for (size_t p = 0; p < node.mesh->primitives_count; p++) {
            const auto& primitive = node.mesh->primitives[p];
            if (primitive.type != cgltf_primitive_type_triangles) {
                continue;
            }
            for (size_t a = 0; a < primitive.attributes_count; a++) {
                if (primitive.attributes[a].type ==
                    cgltf_attribute_type_position) {
                    count++;
                    break;
                }
            }
        }
    }

    cgltf_free(data);
    return count;
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

    const renderer::TextureCreateInfo textureCreateInfo{
        .name          = image->name,
        .path          = "",
        .width         = image->width,
        .height        = image->height,
        .bytesPerPixel = image->bytesPerPixel,
        .data          = image->pixels.data(),
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
        if (const auto id = mesh.material_ids[0]; id != -1) {
            if (!materials[id].diffuse_texname.empty()) {
                parsedMesh.albedo = decodeMaterialTexture(materials[id], path);
            }
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
    };
    stbi_image_free(pixels);
    return image;
}

ModelData Model::parseGltf(const std::string&           path,
                           const std::function<void()>& onMeshParsed) {
    ModelData data;

    core::Timer timer;
    timer.tick();

    const cgltf_options options{};
    cgltf_data*         gltfData = nullptr;
    if (cgltf_parse_file(&options, path.c_str(), &gltfData) !=
        cgltf_result_success) {
        SPONGE_GL_ERROR("Unable to parse gltf model: {}", path);
        return data;
    }

    if (cgltf_load_buffers(&options, gltfData, path.c_str()) !=
        cgltf_result_success) {
        SPONGE_GL_ERROR("Unable to load gltf buffers: {}", path);
        cgltf_free(gltfData);
        return data;
    }

    // Bake each node's world transform into its mesh's vertices so glTF
    // files authored Z-up (or otherwise offset) render the same as their
    // node hierarchy intends, instead of in raw local mesh space.
    for (size_t n = 0; n < gltfData->nodes_count; n++) {
        const auto& node = gltfData->nodes[n];
        if (node.mesh == nullptr) {
            continue;
        }

        std::array<float, 16> worldMatrix{};
        cgltf_node_transform_world(&node, worldMatrix.data());
        const auto transform = glm::make_mat4(worldMatrix.data());

        [[maybe_unused]] const auto* nodeName =
            node.name != nullptr ? node.name : "unnamed";
        for (size_t p = 0; p < node.mesh->primitives_count; p++) {
            SPONGE_GL_INFO("Loading mesh: [{}, primitive {}]", nodeName, p);
            auto parsedMesh =
                parseGltfPrimitive(node.mesh->primitives[p], transform, path);
            if (!parsedMesh) {
                continue;
            }
            data.meshes.emplace_back(std::move(*parsedMesh));
            if (onMeshParsed) {
                onMeshParsed();
            }
        }
    }

    cgltf_free(gltfData);

    timer.tick();
    SPONGE_GL_DEBUG("Parsing time for model: {:.2f} ms",
                    timer.getElapsedSeconds() * secondsToMilliseconds);
    SPONGE_GL_DEBUG("# of meshes    = {}",
                    static_cast<int>(data.meshes.size()));

    return data;
}

std::optional<ParsedMesh>
    Model::parseGltfPrimitive(const cgltf_primitive& primitive,
                              const glm::mat4&       transform,
                              const std::string&     path) {
    if (primitive.type != cgltf_primitive_type_triangles) {
        return std::nullopt;
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
        return std::nullopt;
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

    ParsedMesh parsedMesh;
    if (primitive.material != nullptr) {
        const auto& material = *primitive.material;
        if (material.has_pbr_metallic_roughness) {
            const auto& pbr   = material.pbr_metallic_roughness;
            parsedMesh.albedo = decodeGltfTexture(pbr.base_color_texture, path);
            parsedMesh.metallicRoughness =
                decodeGltfTexture(pbr.metallic_roughness_texture, path);
            parsedMesh.metallicFactor  = pbr.metallic_factor;
            parsedMesh.roughnessFactor = pbr.roughness_factor;
            parsedMesh.uvTransforms.albedo =
                gltfUVTransform(pbr.base_color_texture);
            parsedMesh.uvTransforms.metallicRoughness =
                gltfUVTransform(pbr.metallic_roughness_texture);
        }
        parsedMesh.normal = decodeGltfTexture(material.normal_texture, path);
        parsedMesh.occlusion =
            decodeGltfTexture(material.occlusion_texture, path);
        parsedMesh.emissive =
            decodeGltfTexture(material.emissive_texture, path);
        parsedMesh.uvTransforms.normal =
            gltfUVTransform(material.normal_texture);
        parsedMesh.uvTransforms.occlusion =
            gltfUVTransform(material.occlusion_texture);
        parsedMesh.uvTransforms.emissive =
            gltfUVTransform(material.emissive_texture);
    }

    parsedMesh.vertices = std::move(vertices);
    parsedMesh.indices  = std::move(indices);
    return parsedMesh;
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
    return UVTransform{
        .offset = glm::vec2(t.offset[0], t.offset[1]),
        .scale  = glm::vec2(t.scale[0], t.scale[1]),
    };
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

std::optional<ParsedImage>
    Model::decodeGltfTexture(const cgltf_texture_view& textureView,
                             const std::string&        path) {
    const auto* texture = textureView.texture;
    if (texture == nullptr || texture->image == nullptr) {
        return std::nullopt;
    }

    const auto* image = texture->image;
    if (image->buffer_view == nullptr) {
        SPONGE_GL_WARN("Unsupported gltf image source (expected buffer view)");
        return std::nullopt;
    }

    const auto* bytes = cgltf_buffer_view_data(image->buffer_view);
    const auto  size  = static_cast<int>(image->buffer_view->size);

    // Cache key: path + byte range, not the cgltf_image* address — that's
    // freed by cgltf_free() and gets reused across unrelated model loads,
    // causing collisions.
    //
    // No cross-material decode dedup; AssetManager::createTexture still
    // dedups the GL upload, so a repeat decode only costs CPU time.
    const auto name = path + "#" + std::to_string(image->buffer_view->offset) +
                      "_" + std::to_string(image->buffer_view->size);

    SPONGE_GL_INFO("Loading texture: [{}]", name);

    int   width         = 0;
    int   height        = 0;
    int   bytesPerPixel = 0;
    auto* pixels =
        stbi_load_from_memory(bytes, size, &width, &height, &bytesPerPixel, 0);
    if (pixels == nullptr) {
        SPONGE_GL_ERROR("Unable to decode gltf image: {}",
                        stbi_failure_reason());
        return std::nullopt;
    }

    ParsedImage decoded{
        .name          = name,
        .width         = static_cast<uint32_t>(width),
        .height        = static_cast<uint32_t>(height),
        .bytesPerPixel = static_cast<uint32_t>(bytesPerPixel),
        .pixels        = copyPixels(pixels, width, height, bytesPerPixel),
    };
    stbi_image_free(pixels);
    return decoded;
}

void Model::render(const std::shared_ptr<renderer::Shader>& shader) const {
    SPONGE_PROFILE;
    SPONGE_PROFILE_GPU("render model");

    for (auto&& mesh : meshes) {
        mesh->render(shader);
    }
}
}  // namespace sponge::platform::opengl::scene
