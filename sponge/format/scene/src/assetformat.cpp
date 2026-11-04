#include "assetformat.hpp"

#include "modeldata.hpp"
#include "readbytes.hpp"
#include "vertex.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cfloat>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace {
using sponge::scene::MeshCluster;
using sponge::scene::PackedVertex;
using sponge::scene::ParsedImage;
using sponge::scene::ParsedMesh;
using sponge::scene::asset::Header;
using sponge::scene::asset::magic;
using sponge::scene::asset::textureSlots;
using sponge::scene::asset::version;

// A glm type and its plain-array twin in the generated header hold the same
// floats in the same order. The static_asserts in assetformat.hpp pin the
// sizes and offsets.
template <typename To, typename From>
void copyBytes(To& to, const From& from) {
    static_assert(sizeof(To) == sizeof(From));
    static_assert(std::is_trivially_copyable_v<To>);
    static_assert(std::is_trivially_copyable_v<From>);
    std::memcpy(&to, &from, sizeof(To));
}

// Texture slot order in MeshEntry::textureIndex. Works for const and
// non-const meshes.
template <typename M>
auto slotsOf(M& mesh) {
    return std::array{ &mesh.albedo,
                       &mesh.normal,
                       &mesh.occlusion,
                       &mesh.emissive,
                       &mesh.metallicRoughness,
                       &mesh.diffuseTransmissionMap };
}

// Checks magic, version and vertex layout. Returns nullptr on rejection.
const Header* validate(const std::vector<uint8_t>& bytes,
                       const std::string& path, std::string& error) {
    if (bytes.size() < sizeof(Header)) {
        error = fmt::format(
            "Unable to read baked model, or it is truncated: {}", path);
        return nullptr;
    }

    const auto* header = reinterpret_cast<const Header*>(bytes.data());
    if (std::memcmp(header->magic, magic.data(), magic.size()) != 0) {
        error = fmt::format("Not a baked model: {}", path);
        return nullptr;
    }
    if (header->version != version) {
        error = fmt::format("Baked model version {}, expected {}: {}",
                            header->version, version, path);
        return nullptr;
    }
    if (header->vertexSize != sizeof(PackedVertex)) {
        error = fmt::format("Baked model vertex size {}, expected {}: {}",
                            header->vertexSize, sizeof(PackedVertex), path);
        return nullptr;
    }
    return header;
}
}  // namespace

namespace sponge::scene::asset {

ModelData read(const std::string& path, std::string& error) {
    const auto  bytes  = readBytes(path);
    const auto* header = validate(bytes, path, error);
    if (header == nullptr) {
        return {};
    }

    const auto tablesEnd = sizeof(Header) +
                           (sizeof(MeshEntry) * header->meshCount) +
                           (sizeof(TextureEntry) * header->textureCount);
    if (bytes.size() < tablesEnd) {
        error = fmt::format("Baked model entry tables are truncated: {}", path);
        return {};
    }

    const auto* meshEntries =
        reinterpret_cast<const MeshEntry*>(bytes.data() + sizeof(Header));
    const auto* textureEntries = reinterpret_cast<const TextureEntry*>(
        bytes.data() + sizeof(Header) +
        (sizeof(MeshEntry) * header->meshCount));

    ModelData data;

    // One copy per unique texture, shared by index across the meshes below.
    data.images.resize(header->textureCount);
    for (uint32_t i = 0; i < header->textureCount; i++) {
        const auto& entry = textureEntries[i];
        if (entry.offset + entry.size > bytes.size()) {
            error = fmt::format("Baked texture {} runs past the end of {}", i,
                                path);
            return {};
        }
        data.images[i].name = path + "#" + std::to_string(i);
        data.images[i].ktx2.assign(bytes.begin() + entry.offset,
                                   bytes.begin() + entry.offset + entry.size);
    }

    data.meshes.resize(header->meshCount);
    for (uint32_t i = 0; i < header->meshCount; i++) {
        const auto& entry = meshEntries[i];
        auto&       mesh  = data.meshes[i];

        const auto vertexBytes = entry.vertexCount * sizeof(PackedVertex);
        const auto indexBytes  = entry.indexCount * sizeof(uint32_t);
        if (entry.vertexOffset + vertexBytes > bytes.size() ||
            entry.indexOffset + indexBytes > bytes.size()) {
            error =
                fmt::format("Baked mesh {} runs past the end of {}", i, path);
            return {};
        }

        mesh.packedVertices.resize(entry.vertexCount);
        std::memcpy(mesh.packedVertices.data(),
                    bytes.data() + entry.vertexOffset, vertexBytes);
        mesh.indices.resize(entry.indexCount);
        std::memcpy(mesh.indices.data(), bytes.data() + entry.indexOffset,
                    indexBytes);

        const auto clusterBytes = entry.clusterCount * sizeof(MeshCluster);
        if (entry.clusterCount == 0 ||
            entry.clusterOffset + clusterBytes > bytes.size()) {
            error =
                fmt::format("Baked mesh {} has no cluster table: {}", i, path);
            return {};
        }
        mesh.clusters.resize(entry.clusterCount);
        std::memcpy(mesh.clusters.data(), bytes.data() + entry.clusterOffset,
                    clusterBytes);
        // Every range must be inside the indices, and LOD0 must be one block
        // at the start: Mesh draws LOD0 as one range.
        uint32_t lod0End  = 0;
        bool     pastLod0 = false;
        for (const auto& cluster : mesh.clusters) {
            const bool lod0 = cluster.depth == 0;
            if (uint64_t{ cluster.firstIndex } + cluster.indexCount >
                    entry.indexCount ||
                (lod0 && (pastLod0 || cluster.firstIndex != lod0End))) {
                error = fmt::format("Baked mesh {} has a bad cluster table: {}",
                                    i, path);
                return {};
            }
            lod0End += lod0 ? cluster.indexCount : 0;
            pastLod0 = pastLod0 || !lod0;
        }

        size_t slot = 0;
        for (auto* image : slotsOf(mesh)) {
            const auto id = entry.textureIndex[slot];
            if (id >= 0 && id < static_cast<int32_t>(data.images.size())) {
                *image = static_cast<uint32_t>(id);
            }
            slot++;
        }

        copyBytes(mesh.baseColorFactor, entry.baseColorFactor);
        mesh.metallicFactor           = entry.metallicFactor;
        mesh.roughnessFactor          = entry.roughnessFactor;
        mesh.clearcoatFactor          = entry.clearcoatFactor;
        mesh.clearcoatRoughnessFactor = entry.clearcoatRoughnessFactor;
        // A value past Blend is a corrupt file; draw it as opaque.
        mesh.alphaMode =
            entry.alphaMode <= static_cast<uint32_t>(AlphaMode::Blend) ?
                static_cast<AlphaMode>(entry.alphaMode) :
                AlphaMode::Opaque;
        mesh.alphaCutoff = entry.alphaCutoff;
        mesh.doubleSided = entry.doubleSided != 0;
        mesh.doubleSided = entry.doubleSided != 0;
        copyBytes(mesh.diffuseTransmission, entry.diffuseTransmission);
        copyBytes(mesh.uvTransforms, entry.uvTransforms);
    }

    return data;
}

std::size_t readMeshCount(const std::string& path, std::string& error) {
    // Header only: this is called before the load to size a progress bar, and
    // read() below pulls the rest.
    const auto  bytes  = readBytes(path, sizeof(Header));
    const auto* header = validate(bytes, path, error);
    return header == nullptr ? 0 : header->meshCount;
}

}  // namespace sponge::scene::asset
