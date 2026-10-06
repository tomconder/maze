#include "platform/opengl/scene/model.hpp"

#include "assetformat.hpp"
#include "debug/profiler.hpp"
#include "logging/log.hpp"
#include "platform/opengl/debug/profiler.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace {
// Slang-generated layout locations for pbr.vert.slang
constexpr uint32_t positionLoc = 0;
constexpr uint32_t texCoordLoc = 1;
constexpr uint32_t normalLoc   = 2;
constexpr uint32_t tangentLoc  = 3;
}  // namespace

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;

Model::Model(const ModelCreateInfo& createInfo) {
    assert(!createInfo.path.empty());

    auto data = parse(createInfo);
    for (auto& parsedMesh : data.meshes) {
        auto mesh = buildMesh(std::move(parsedMesh), data.images);
        numIndices += mesh->getNumIndices();
        numVertices += mesh->getNumVertices();
        meshes.emplace_back(std::move(mesh));
    }
    packMeshes();
}

Model::Model(std::vector<std::shared_ptr<Mesh>>&& builtMeshes) {
    for (const auto& mesh : builtMeshes) {
        numIndices += mesh->getNumIndices();
        numVertices += mesh->getNumVertices();
    }
    meshes = std::move(builtMeshes);
    packMeshes();
}

void Model::packMeshes() {
    if (meshes.empty()) {
        return;
    }

    std::vector<sponge::scene::Vertex> vertices;
    std::vector<uint32_t>              indices;
    vertices.reserve(numVertices);
    indices.reserve(numIndices);
    for (const auto& mesh : meshes) {
        maskedMeshes = maskedMeshes || mesh->isMasked();
        mesh->setDrawRange(static_cast<int32_t>(vertices.size()),
                           static_cast<uint32_t>(indices.size()));
        const auto meshVertices = mesh->getVertices();
        const auto meshIndices  = mesh->getIndices();
        vertices.insert(vertices.end(), meshVertices.begin(),
                        meshVertices.end());
        indices.insert(indices.end(), meshIndices.begin(), meshIndices.end());
        mesh->freeGeometry();
    }

    vbo = std::make_unique<renderer::VertexBuffer>(
        vertices.data(), vertices.size() * sizeof(sponge::scene::Vertex));
    ebo = std::make_unique<renderer::IndexBuffer>(
        indices.data(), indices.size() * sizeof(uint32_t));
    vao = std::make_unique<renderer::VertexArray>();
    vao->setVertexBuffer(*vbo, sizeof(sponge::scene::Vertex));
    vao->setIndexBuffer(*ebo);
    vao->addAttribute(positionLoc, 3,
                      offsetof(sponge::scene::Vertex, position));
    vao->addAttribute(texCoordLoc, 2,
                      offsetof(sponge::scene::Vertex, texCoords));
    vao->addAttribute(normalLoc, 3, offsetof(sponge::scene::Vertex, normal));
    vao->addAttribute(tangentLoc, 4, offsetof(sponge::scene::Vertex, tangent));
}

ModelData Model::parse(const ModelCreateInfo& createInfo) {
    assert(!createInfo.path.empty());
    SPONGE_GL_INFO("Loading model file: [{}, {}]", createInfo.name,
                   createInfo.path);

    std::string error;
    auto        data = sponge::scene::asset::read(
        createInfo.assetsFolder + createInfo.path, error);
    if (!error.empty()) {
        SPONGE_GL_ERROR("{}", error);
    }
    return data;
}

std::size_t Model::countMeshes(const ModelCreateInfo& createInfo) {
    std::string error;
    const auto  count = sponge::scene::asset::readMeshCount(
        createInfo.assetsFolder + createInfo.path, error);
    if (!error.empty()) {
        SPONGE_GL_ERROR("{}", error);
    }
    return count;
}

std::shared_ptr<Mesh> Model::buildMesh(ParsedMesh&&                 parsedMesh,
                                       std::span<const ParsedImage> images) {
    // captured before the moves below: arg evaluation order is unspecified
    const auto vertexCount = parsedMesh.vertices.size();
    const auto indexCount  = parsedMesh.indices.size();

    std::vector<std::shared_ptr<renderer::Texture>> textures;
    if (auto albedo = buildTexture(parsedMesh.albedo, images)) {
        textures.emplace_back(std::move(albedo));
    }

    auto mesh = std::make_shared<Mesh>(
        std::move(parsedMesh.vertices), vertexCount,
        std::move(parsedMesh.indices), indexCount, std::move(textures),
        buildTexture(parsedMesh.normal, images),
        buildTexture(parsedMesh.occlusion, images),
        buildTexture(parsedMesh.emissive, images),
        buildTexture(parsedMesh.metallicRoughness, images),
        buildTexture(parsedMesh.diffuseTransmissionMap, images),
        parsedMesh.metallicFactor, parsedMesh.roughnessFactor,
        parsedMesh.clearcoatFactor, parsedMesh.clearcoatRoughnessFactor,
        parsedMesh.diffuseTransmission, parsedMesh.baseColorFactor,
        parsedMesh.alphaCutoff, parsedMesh.uvTransforms);
    return mesh;
}

std::shared_ptr<renderer::Texture>
    Model::buildTexture(const std::optional<uint32_t>      image,
                        const std::span<const ParsedImage> images) {
    if (!image) {
        return nullptr;
    }
    assert(*image < images.size());
    const auto& parsed = images[*image];

    // Every image in a baked model is a whole KTX2 file; the raw-pixel
    // fields of ParsedImage are the converter's side of the struct.
    const renderer::TextureCreateInfo textureCreateInfo{
        .name = parsed.name,
        .path = "",
        .ktx2 = parsed.ktx2,
    };
    return AssetManager::createTexture(textureCreateInfo);
}

void Model::render(const std::shared_ptr<renderer::Shader>& shader) const {
    SPONGE_PROFILE;
    SPONGE_PROFILE_GPU("render model");

    if (!vao) {
        return;
    }
    vao->bind();
    for (auto&& mesh : meshes) {
        mesh->draw(shader);
    }
}

void Model::render(const std::shared_ptr<renderer::Shader>& shader,
                   const std::span<const uint8_t>           meshVisible,
                   const AlphaPass                          pass) const {
    SPONGE_PROFILE;
    SPONGE_PROFILE_GPU("render model culled");

    if (!vao) {
        return;
    }
    vao->bind();
    for (size_t i = 0; i < meshes.size(); i++) {
        const bool masked = meshes[i]->isMasked();
        if ((pass == AlphaPass::Opaque && masked) ||
            (pass == AlphaPass::Masked && !masked)) {
            continue;
        }
        if (meshVisible.empty() || meshVisible[i] != 0) {
            meshes[i]->draw(shader, pass == AlphaPass::Masked);
        }
    }
}
}  // namespace sponge::platform::opengl::scene
