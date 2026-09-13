#include "gltfimport.hpp"

#include "logging/log.hpp"
#include "scene/mesh.hpp"
#include "scene/modeldata.hpp"
#include "scene/tangents.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cgltf.h>
#include <stb_image.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {
std::vector<uint8_t> copyPixels(const uint8_t* pixels, const int width,
                                const int height, const int bytesPerPixel) {
    const auto* begin = pixels;
    const auto* end =
        pixels + (static_cast<size_t>(width) * height * bytesPerPixel);
    return { begin, end };
}
}  // namespace

namespace assetconv::gltf {
using sponge::scene::computeTangents;
using sponge::scene::ModelData;
using sponge::scene::ParsedImage;
using sponge::scene::ParsedMesh;
using sponge::scene::UVTransform;
using sponge::scene::Vertex;
namespace {
UVTransform uvTransformOf(const cgltf_texture_view& textureView) {
    if (!textureView.has_transform) {
        return {};
    }
    const auto& t = textureView.transform;
    if (t.rotation != 0.F) {
        SPONGE_WARN(
            "KHR_texture_transform rotation is not supported; ignoring");
    }
    return UVTransform{
        .offset = glm::vec2(t.offset[0], t.offset[1]),
        .scale  = glm::vec2(t.scale[0], t.scale[1]),
    };
}

std::optional<ParsedImage> decodeTexture(const cgltf_texture_view& textureView,
                                         const std::string&        path) {
    const auto* texture = textureView.texture;
    if (texture == nullptr || texture->image == nullptr) {
        return std::nullopt;
    }

    const auto* image = texture->image;
    if (image->buffer_view == nullptr) {
        SPONGE_WARN("Unsupported gltf image source (expected buffer view)");
        return std::nullopt;
    }

    const auto* bytes = cgltf_buffer_view_data(image->buffer_view);
    const auto  size  = static_cast<int>(image->buffer_view->size);

    // Path and byte range: the .spnga writer stores one copy per name.
    const auto name = path + "#" + std::to_string(image->buffer_view->offset) +
                      "_" + std::to_string(image->buffer_view->size);

    SPONGE_INFO("Loading texture: [{}]", name);

    int   width         = 0;
    int   height        = 0;
    int   bytesPerPixel = 0;
    auto* pixels =
        stbi_load_from_memory(bytes, size, &width, &height, &bytesPerPixel, 0);
    if (pixels == nullptr) {
        SPONGE_ERROR("Unable to decode gltf image: {}", stbi_failure_reason());
        return std::nullopt;
    }

    ParsedImage decoded{
        .name          = name,
        .width         = static_cast<uint32_t>(width),
        .height        = static_cast<uint32_t>(height),
        .bytesPerPixel = static_cast<uint32_t>(bytesPerPixel),
        .pixels        = copyPixels(pixels, width, height, bytesPerPixel),
        .ktx2          = {},
    };
    stbi_image_free(pixels);
    return decoded;
}

std::optional<ParsedMesh> parsePrimitive(const cgltf_primitive& primitive,
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
            parsedMesh.albedo = decodeTexture(pbr.base_color_texture, path);
            parsedMesh.metallicRoughness =
                decodeTexture(pbr.metallic_roughness_texture, path);
            parsedMesh.metallicFactor  = pbr.metallic_factor;
            parsedMesh.roughnessFactor = pbr.roughness_factor;
            parsedMesh.uvTransforms.albedo =
                uvTransformOf(pbr.base_color_texture);
            parsedMesh.uvTransforms.metallicRoughness =
                uvTransformOf(pbr.metallic_roughness_texture);
        }
        parsedMesh.normal    = decodeTexture(material.normal_texture, path);
        parsedMesh.occlusion = decodeTexture(material.occlusion_texture, path);
        parsedMesh.emissive  = decodeTexture(material.emissive_texture, path);
        parsedMesh.uvTransforms.normal = uvTransformOf(material.normal_texture);
        parsedMesh.uvTransforms.occlusion =
            uvTransformOf(material.occlusion_texture);
        parsedMesh.uvTransforms.emissive =
            uvTransformOf(material.emissive_texture);
    }

    parsedMesh.vertices = std::move(vertices);
    parsedMesh.indices  = std::move(indices);
    return parsedMesh;
}
}  // namespace

sponge::scene::ModelData parse(const std::string& path) {
    ModelData data;

    constexpr cgltf_options options{};
    cgltf_data*             gltfData = nullptr;
    if (cgltf_parse_file(&options, path.c_str(), &gltfData) !=
        cgltf_result_success) {
        SPONGE_ERROR("Unable to parse gltf model: {}", path);
        return data;
    }

    if (cgltf_load_buffers(&options, gltfData, path.c_str()) !=
        cgltf_result_success) {
        SPONGE_ERROR("Unable to load gltf buffers: {}", path);
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

        for (size_t p = 0; p < node.mesh->primitives_count; p++) {
            SPONGE_INFO("Loading mesh: [primitive {}]", p);
            auto parsedMesh =
                parsePrimitive(node.mesh->primitives[p], transform, path);
            if (!parsedMesh) {
                continue;
            }
            data.meshes.emplace_back(std::move(*parsedMesh));
        }
    }

    cgltf_free(gltfData);

    SPONGE_DEBUG("# of meshes    = {}", static_cast<int>(data.meshes.size()));

    return data;
}

}  // namespace assetconv::gltf
