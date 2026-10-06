#include "gltfimport.hpp"

#include "modeldata.hpp"
#include "readbytes.hpp"
#include "tangents.hpp"
#include "vertex.hpp"

#include <fmt/base.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cgltf.h>
#include <stb_image.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
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
using sponge::scene::AlphaMode;
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
        fmt::println(stderr, "assetconv: KHR_texture_transform rotation is "
                             "not supported; ignoring");
    }
    return UVTransform{
        .offset = glm::vec2(t.offset[0], t.offset[1]),
        .scale  = glm::vec2(t.scale[0], t.scale[1]),
    };
}

std::optional<ParsedImage> decodeImage(const uint8_t*     bytes,
                                       const std::size_t  byteCount,
                                       const std::string& name) {
    const auto size = static_cast<int>(byteCount);

    int   width         = 0;
    int   height        = 0;
    int   bytesPerPixel = 0;
    auto* pixels =
        stbi_load_from_memory(bytes, size, &width, &height, &bytesPerPixel, 0);
    if (pixels == nullptr) {
        fmt::println(stderr, "assetconv: unable to decode {}: {}", name,
                     stbi_failure_reason());
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

bool isDataUri(const char* uri) {
    return std::string_view(uri).starts_with("data:");
}

// The bytes of an image that is not in a buffer view: a base64 data URI, or a
// file named relative to the glTF file. Empty on any failure.
std::vector<uint8_t> readImageUri(const char* uri, const std::string& path) {
    if (isDataUri(uri)) {
        const std::string_view whole(uri);
        const auto             comma = whole.find(',');
        if (comma == std::string_view::npos) {
            return {};
        }
        const auto              base64  = whole.substr(comma + 1);
        const auto              padding = base64.ends_with("==") ? 2U :
                                          base64.ends_with('=')  ? 1U :
                                                                   0U;
        const auto              size    = (base64.size() / 4 * 3) - padding;
        constexpr cgltf_options options{};
        void*                   data = nullptr;
        if (cgltf_load_buffer_base64(&options, size, base64.data(), &data) !=
            cgltf_result_success) {
            return {};
        }
        const auto*          begin = static_cast<const uint8_t*>(data);
        std::vector<uint8_t> bytes(begin, begin + size);
        std::free(data);
        return bytes;
    }

    std::string decoded(uri);
    decoded.resize(cgltf_decode_uri(decoded.data()));
    return sponge::scene::readBytes(
        (std::filesystem::path(path).parent_path() / decoded).string());
}

// Index into ModelData::images by image name, so an image shared by several
// materials decodes once. Empty when the decode failed.
using ImageIndex = std::map<std::string, std::optional<uint32_t>>;

std::optional<uint32_t> decodeTexture(const cgltf_texture_view& textureView,
                                      const std::string& path, ModelData& data,
                                      ImageIndex& imageIndex) {
    const auto* texture = textureView.texture;
    if (texture == nullptr || texture->image == nullptr) {
        return std::nullopt;
    }

    const auto* image = texture->image;
    if (image->buffer_view == nullptr && image->uri == nullptr) {
        fmt::println(stderr, "assetconv: {}: gltf image has no source", path);
        return std::nullopt;
    }

    // Path and byte range, or path and URI: one image per name.
    const auto name = image->buffer_view != nullptr ?
                          path + "#" +
                              std::to_string(image->buffer_view->offset) + "_" +
                              std::to_string(image->buffer_view->size) :
                          path + "#" + image->uri;

    auto [entry, inserted] = imageIndex.try_emplace(name);
    if (inserted) {
        std::vector<uint8_t> external;
        const uint8_t*       bytes     = nullptr;
        std::size_t          byteCount = 0;
        if (image->buffer_view != nullptr) {
            bytes     = cgltf_buffer_view_data(image->buffer_view);
            byteCount = image->buffer_view->size;
        } else {
            external  = readImageUri(image->uri, path);
            bytes     = external.data();
            byteCount = external.size();
        }
        if (byteCount == 0) {
            fmt::println(stderr, "assetconv: {}: unable to read image {}", path,
                         name);
        } else if (auto decoded = decodeImage(bytes, byteCount, name)) {
            entry->second = static_cast<uint32_t>(data.images.size());
            data.images.push_back(std::move(*decoded));
        }
    }
    return entry->second;
}

std::optional<ParsedMesh> parsePrimitive(const cgltf_primitive& primitive,
                                         const glm::mat4&       transform,
                                         const std::string&     path,
                                         ModelData&             data,
                                         ImageIndex&            imageIndex) {
    if (primitive.type != cgltf_primitive_type_triangles) {
        return std::nullopt;
    }

    const auto normalMatrix  = glm::mat3(transpose(inverse(transform)));
    const auto tangentMatrix = glm::mat3(transform);

    const cgltf_accessor* positionAccessor = nullptr;
    const cgltf_accessor* normalAccessor   = nullptr;
    const cgltf_accessor* tangentAccessor  = nullptr;
    const cgltf_accessor* texcoordAccessor = nullptr;

    for (size_t a = 0; a < primitive.attributes_count; a++) {
        const auto& attribute = primitive.attributes[a];
        if (attribute.type == cgltf_attribute_type_position) {
            positionAccessor = attribute.data;
        } else if (attribute.type == cgltf_attribute_type_normal) {
            normalAccessor = attribute.data;
        } else if (attribute.type == cgltf_attribute_type_tangent) {
            tangentAccessor = attribute.data;
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

        if (tangentAccessor != nullptr) {
            glm::vec4 tangent(0.F);
            cgltf_accessor_read_float(tangentAccessor, i, &tangent.x, 4);
            vertices[i].tangent = glm::vec4(
                normalize(tangentMatrix * glm::vec3(tangent)), tangent.w);
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
        // A vertex shared by several triangles keeps only the last face
        // normal written, so shading is wrong wherever vertices are shared.
        fmt::println(stderr,
                     "assetconv: {}: primitive has no normals; generated "
                     "normals are wrong on shared vertices",
                     path);
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

    // glTF ignores authored tangents when normals are missing, since they
    // were built against normals the file does not have.
    if (tangentAccessor == nullptr || normalAccessor == nullptr) {
        computeTangents(vertices, indices);
    }

    ParsedMesh parsedMesh;
    if (primitive.material != nullptr) {
        const auto& material = *primitive.material;
        if (material.has_pbr_metallic_roughness) {
            const auto& pbr = material.pbr_metallic_roughness;
            parsedMesh.albedo =
                decodeTexture(pbr.base_color_texture, path, data, imageIndex);
            parsedMesh.metallicRoughness = decodeTexture(
                pbr.metallic_roughness_texture, path, data, imageIndex);
            parsedMesh.baseColorFactor = glm::make_vec4(pbr.base_color_factor);
            parsedMesh.metallicFactor  = pbr.metallic_factor;
            parsedMesh.roughnessFactor = pbr.roughness_factor;
            parsedMesh.uvTransforms.albedo =
                uvTransformOf(pbr.base_color_texture);
            parsedMesh.uvTransforms.metallicRoughness =
                uvTransformOf(pbr.metallic_roughness_texture);
        }
        parsedMesh.doubleSided = material.double_sided != 0;
        parsedMesh.doubleSided = material.double_sided != 0;
        if (material.alpha_mode == cgltf_alpha_mode_mask) {
            parsedMesh.alphaMode   = AlphaMode::Mask;
            parsedMesh.alphaCutoff = material.alpha_cutoff;
        } else if (material.alpha_mode == cgltf_alpha_mode_blend) {
            parsedMesh.alphaMode = AlphaMode::Blend;
        }
        if (material.has_clearcoat) {
            parsedMesh.clearcoatFactor = material.clearcoat.clearcoat_factor;
            parsedMesh.clearcoatRoughnessFactor =
                material.clearcoat.clearcoat_roughness_factor;
        }
        if (material.has_diffuse_transmission) {
            const auto& dt = material.diffuse_transmission;
            parsedMesh.diffuseTransmission =
                glm::vec4(dt.diffuse_transmission_color_factor[0],
                          dt.diffuse_transmission_color_factor[1],
                          dt.diffuse_transmission_color_factor[2],
                          dt.diffuse_transmission_factor);
            parsedMesh.diffuseTransmissionMap = decodeTexture(
                dt.diffuse_transmission_texture, path, data, imageIndex);
            parsedMesh.uvTransforms.diffuseTransmission =
                uvTransformOf(dt.diffuse_transmission_texture);
        }
        parsedMesh.normal =
            decodeTexture(material.normal_texture, path, data, imageIndex);
        parsedMesh.occlusion =
            decodeTexture(material.occlusion_texture, path, data, imageIndex);
        parsedMesh.emissive =
            decodeTexture(material.emissive_texture, path, data, imageIndex);
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
    ModelData  data;
    ImageIndex imageIndex;

    constexpr cgltf_options options{};
    cgltf_data*             gltfData = nullptr;
    if (cgltf_parse_file(&options, path.c_str(), &gltfData) !=
        cgltf_result_success) {
        fmt::println(stderr, "assetconv: unable to parse {}", path);
        return data;
    }

    if (cgltf_load_buffers(&options, gltfData, path.c_str()) !=
        cgltf_result_success) {
        fmt::println(stderr, "assetconv: unable to load buffers of {}", path);
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
            auto parsedMesh = parsePrimitive(node.mesh->primitives[p],
                                             transform, path, data, imageIndex);
            if (!parsedMesh) {
                continue;
            }
            data.meshes.emplace_back(std::move(*parsedMesh));
        }
    }

    cgltf_free(gltfData);

    return data;
}

std::vector<std::filesystem::path> dependencies(const std::string& path) {
    constexpr cgltf_options options{};
    cgltf_data*             gltfData = nullptr;
    if (cgltf_parse_file(&options, path.c_str(), &gltfData) !=
        cgltf_result_success) {
        return {};
    }

    const auto folder = std::filesystem::path(path).parent_path();
    const auto add    = [&](std::vector<std::filesystem::path>& files,
                            const char*                         uri) {
        if (uri == nullptr || isDataUri(uri)) {
            return;
        }
        std::string decoded(uri);
        decoded.resize(cgltf_decode_uri(decoded.data()));
        files.push_back(folder / decoded);
    };

    std::vector<std::filesystem::path> files;
    for (size_t i = 0; i < gltfData->buffers_count; i++) {
        add(files, gltfData->buffers[i].uri);
    }
    for (size_t i = 0; i < gltfData->images_count; i++) {
        add(files, gltfData->images[i].uri);
    }
    cgltf_free(gltfData);
    return files;
}

}  // namespace assetconv::gltf
