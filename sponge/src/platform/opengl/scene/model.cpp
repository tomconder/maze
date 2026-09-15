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
}

Model::Model(std::vector<std::shared_ptr<Mesh>>&& builtMeshes) {
    for (const auto& mesh : builtMeshes) {
        numIndices += mesh->getNumIndices();
        numVertices += mesh->getNumVertices();
    }
    meshes = std::move(builtMeshes);
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
        parsedMesh.metallicFactor, parsedMesh.roughnessFactor,
        parsedMesh.uvTransforms);
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

    for (auto&& mesh : meshes) {
        mesh->render(shader);
    }
}
}  // namespace sponge::platform::opengl::scene
