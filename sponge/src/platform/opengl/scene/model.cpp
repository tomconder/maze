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
// Moves face culling to `wanted` and records it, so a run of meshes that
// agree makes no GL call.
void setCulling(bool& current, const bool wanted) {
    if (current == wanted) {
        return;
    }
    if (wanted) {
        glEnable(GL_CULL_FACE);
    } else {
        glDisable(GL_CULL_FACE);
    }
    current = wanted;
}

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
        maskedMeshes = maskedMeshes || mesh->alphaPass() == AlphaPass::Masked;
        blendedMeshes =
            blendedMeshes || mesh->alphaPass() == AlphaPass::Blended;
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
    auto       clusters    = std::move(parsedMesh.clusters);

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
        parsedMesh.alphaMode, parsedMesh.alphaCutoff, parsedMesh.doubleSided,
        parsedMesh.uvTransforms);
    mesh->setClusters(std::move(clusters));
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

size_t Model::render(const std::shared_ptr<renderer::Shader>& shader,
                     const AlphaPass                          pass) const {
    return render(shader, {}, pass);
}

size_t Model::render(
    const std::shared_ptr<renderer::Shader>& shader,
    const std::span<const uint8_t> meshVisible, const AlphaPass pass,
    const std::span<const sponge::scene::MeshCut>    meshCuts,
    const std::span<const sponge::scene::IndexRange> ranges) const {
    SPONGE_PROFILE;
    SPONGE_PROFILE_GPU("render model culled");

    if (!vao) {
        return 0;
    }
    vao->bind();
    // Face culling is on by default (RendererAPI). A double-sided mesh turns it
    // off for its own draw; the run ends with it back on.
    bool   culling = true;
    size_t drawn   = 0;
    for (size_t i = 0; i < meshes.size(); i++) {
        if (!meshes[i]->drawnIn(pass)) {
            continue;
        }
        if (meshVisible.empty() || meshVisible[i] != 0) {
            setCulling(culling, !meshes[i]->isDoubleSided());
            const auto cut =
                meshCuts.empty() ? sponge::scene::MeshCut{} : meshCuts[i];
            drawn += meshes[i]->draw(shader, pass == AlphaPass::Masked,
                                     ranges.subspan(cut.first, cut.count));
        }
    }
    setCulling(culling, true);
    return drawn;
}

void Model::renderMesh(const std::shared_ptr<renderer::Shader>& shader,
                       const size_t                             index) const {
    if (!vao) {
        return;
    }
    vao->bind();
    bool culling = true;
    setCulling(culling, !meshes[index]->isDoubleSided());
    meshes[index]->draw(shader);
    setCulling(culling, true);
}
}  // namespace sponge::platform::opengl::scene
