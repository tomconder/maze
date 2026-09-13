#include "scene/assetformat.hpp"

#include "core/file.hpp"
#include "logging/log.hpp"
#include "scene/mesh.hpp"
#include "scene/modeldata.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
using sponge::scene::ParsedImage;
using sponge::scene::ParsedMesh;
using sponge::scene::Vertex;
using sponge::scene::asset::Header;
using sponge::scene::asset::magic;
using sponge::scene::asset::textureSlots;
using sponge::scene::asset::version;

// Vertex holds a vec4, so blobs start on a 16-byte boundary. Readers may
// then view the buffer in place instead of copying out.
constexpr size_t blobAlignment = 16;

void align(std::vector<uint8_t>& out) {
    out.resize((out.size() + blobAlignment - 1) / blobAlignment *
               blobAlignment);
}

template <typename T>
void append(std::vector<uint8_t>& out, const T& value) {
    const auto* bytes = reinterpret_cast<const uint8_t*>(&value);
    out.insert(out.end(), bytes, bytes + sizeof(T));
}

void appendBytes(std::vector<uint8_t>& out, const void* data,
                 const size_t size) {
    const auto* bytes = static_cast<const uint8_t*>(data);
    out.insert(out.end(), bytes, bytes + size);
}

std::array<const std::optional<ParsedImage>*, textureSlots>
    slotsOf(const ParsedMesh& mesh) {
    return { &mesh.albedo, &mesh.normal, &mesh.occlusion, &mesh.emissive,
             &mesh.metallicRoughness };
}

std::array<std::optional<ParsedImage>*, textureSlots>
    slotsOf(ParsedMesh& mesh) {
    return { &mesh.albedo, &mesh.normal, &mesh.occlusion, &mesh.emissive,
             &mesh.metallicRoughness };
}

// Checks magic, version and vertex layout. Returns nullptr on rejection.
const Header* validate(const std::vector<uint8_t>& bytes,
                       const std::string&          path) {
    if (bytes.size() < sizeof(Header)) {
        SPONGE_ERROR("Unable to read baked model, or it is truncated: {}",
                     path);
        return nullptr;
    }

    const auto* header = reinterpret_cast<const Header*>(bytes.data());
    if (std::memcmp(header->magic, magic, sizeof(magic)) != 0) {
        SPONGE_ERROR("Not a baked model: {}", path);
        return nullptr;
    }
    if (header->version != version) {
        SPONGE_ERROR("Baked model version {}, expected {}: {}", header->version,
                     version, path);
        return nullptr;
    }
    if (header->vertexSize != sizeof(Vertex)) {
        SPONGE_ERROR("Baked model vertex size {}, expected {}: {}",
                     header->vertexSize, sizeof(Vertex), path);
        return nullptr;
    }
    return header;
}
}  // namespace

namespace sponge::scene::asset {

std::vector<uint8_t> write(const std::span<const ParsedMesh> meshes) {
    // Dedup by image name so a texture shared across materials is stored
    // once. Importer names come from the source byte range, so they are
    // stable across bakes.
    std::unordered_map<std::string, int32_t>       textureIds;
    std::vector<const std::vector<uint8_t>*>       textureBlobs;
    std::vector<std::array<int32_t, textureSlots>> meshSlots;
    meshSlots.reserve(meshes.size());

    for (const auto& mesh : meshes) {
        std::array<int32_t, textureSlots> ids{};
        ids.fill(-1);
        size_t slot = 0;
        for (const auto* image : slotsOf(mesh)) {
            if (image->has_value()) {
                const auto [id, inserted] = textureIds.try_emplace(
                    (*image)->name, static_cast<int32_t>(textureBlobs.size()));
                if (inserted) {
                    textureBlobs.emplace_back(&(*image)->ktx2);
                }
                ids[slot] = id->second;
            }
            slot++;
        }
        meshSlots.emplace_back(ids);
    }

    std::vector<uint8_t> out;
    Header               header{
        .magic        = {},
        .version      = version,
        .vertexSize   = sizeof(Vertex),
        .meshCount    = static_cast<uint32_t>(meshes.size()),
        .textureCount = static_cast<uint32_t>(textureBlobs.size()),
    };
    std::memcpy(header.magic, magic, sizeof(magic));
    append(out, header);

    // Entry tables are fixed size, so their file offsets are known before
    // the blobs are laid out; the entries themselves are patched in after.
    const auto meshTableOffset = out.size();
    out.resize(meshTableOffset + (sizeof(MeshEntry) * meshes.size()));
    const auto textureTableOffset = out.size();
    out.resize(textureTableOffset +
               (sizeof(TextureEntry) * textureBlobs.size()));

    std::vector<MeshEntry>    meshEntries(meshes.size());
    std::vector<TextureEntry> textureEntries(textureBlobs.size());

    for (size_t i = 0; i < meshes.size(); i++) {
        const auto& mesh  = meshes[i];
        auto&       entry = meshEntries[i];

        align(out);
        entry.vertexOffset = static_cast<uint32_t>(out.size());
        entry.vertexCount  = static_cast<uint32_t>(mesh.vertices.size());
        appendBytes(out, mesh.vertices.data(),
                    mesh.vertices.size() * sizeof(Vertex));

        align(out);
        entry.indexOffset = static_cast<uint32_t>(out.size());
        entry.indexCount  = static_cast<uint32_t>(mesh.indices.size());
        appendBytes(out, mesh.indices.data(),
                    mesh.indices.size() * sizeof(uint32_t));

        std::ranges::copy(meshSlots[i], std::begin(entry.textureIndex));
        entry.metallicFactor  = mesh.metallicFactor;
        entry.roughnessFactor = mesh.roughnessFactor;
        entry.uvTransforms    = mesh.uvTransforms;
    }

    for (size_t i = 0; i < textureBlobs.size(); i++) {
        align(out);
        textureEntries[i].offset = static_cast<uint32_t>(out.size());
        textureEntries[i].size = static_cast<uint32_t>(textureBlobs[i]->size());
        appendBytes(out, textureBlobs[i]->data(), textureBlobs[i]->size());
    }

    std::memcpy(out.data() + meshTableOffset, meshEntries.data(),
                sizeof(MeshEntry) * meshEntries.size());
    std::memcpy(out.data() + textureTableOffset, textureEntries.data(),
                sizeof(TextureEntry) * textureEntries.size());

    return out;
}

ModelData read(const std::string& path) {
    const auto  bytes  = core::File::readBytes(path);
    const auto* header = validate(bytes, path);
    if (header == nullptr) {
        return {};
    }

    const auto tablesEnd = sizeof(Header) +
                           (sizeof(MeshEntry) * header->meshCount) +
                           (sizeof(TextureEntry) * header->textureCount);
    if (bytes.size() < tablesEnd) {
        SPONGE_ERROR("Baked model entry tables are truncated: {}", path);
        return {};
    }

    const auto* meshEntries =
        reinterpret_cast<const MeshEntry*>(bytes.data() + sizeof(Header));
    const auto* textureEntries = reinterpret_cast<const TextureEntry*>(
        bytes.data() + sizeof(Header) +
        (sizeof(MeshEntry) * header->meshCount));

    // One copy per unique texture, shared by index across the meshes below.
    std::vector<ParsedImage> textures(header->textureCount);
    for (uint32_t i = 0; i < header->textureCount; i++) {
        const auto& entry = textureEntries[i];
        if (entry.offset + entry.size > bytes.size()) {
            SPONGE_ERROR("Baked texture {} runs past the end of {}", i, path);
            return {};
        }
        textures[i].name = path + "#" + std::to_string(i);
        textures[i].ktx2.assign(bytes.begin() + entry.offset,
                                bytes.begin() + entry.offset + entry.size);
    }

    ModelData data;
    data.meshes.resize(header->meshCount);
    for (uint32_t i = 0; i < header->meshCount; i++) {
        const auto& entry = meshEntries[i];
        auto&       mesh  = data.meshes[i];

        const auto vertexBytes = entry.vertexCount * sizeof(Vertex);
        const auto indexBytes  = entry.indexCount * sizeof(uint32_t);
        if (entry.vertexOffset + vertexBytes > bytes.size() ||
            entry.indexOffset + indexBytes > bytes.size()) {
            SPONGE_ERROR("Baked mesh {} runs past the end of {}", i, path);
            return {};
        }

        mesh.vertices.resize(entry.vertexCount);
        std::memcpy(mesh.vertices.data(), bytes.data() + entry.vertexOffset,
                    vertexBytes);
        mesh.indices.resize(entry.indexCount);
        std::memcpy(mesh.indices.data(), bytes.data() + entry.indexOffset,
                    indexBytes);

        size_t slot = 0;
        for (auto* image : slotsOf(mesh)) {
            const auto id = entry.textureIndex[slot];
            if (id >= 0 && id < static_cast<int32_t>(textures.size())) {
                *image = textures[id];
            }
            slot++;
        }

        mesh.metallicFactor  = entry.metallicFactor;
        mesh.roughnessFactor = entry.roughnessFactor;
        mesh.uvTransforms    = entry.uvTransforms;
    }

    return data;
}

std::size_t readMeshCount(const std::string& path) {
    // Header only: this is called before the load to size a progress bar, and
    // read() below pulls the rest.
    const auto  bytes  = core::File::readBytes(path, sizeof(Header));
    const auto* header = validate(bytes, path);
    return header == nullptr ? 0 : header->meshCount;
}

}  // namespace sponge::scene::asset
