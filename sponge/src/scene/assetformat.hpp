#pragma once

#include "scene/mesh.hpp"
#include "scene/modeldata.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

// Baked model container (.spnga). Written by tools/assetconv, read by
// Model::parse(). One fread, then views over the buffer.
//
// magic and version are the first two fields and stay at offsets 0 and 8
// forever, so any future revision can identify any file before trusting a
// single other byte. Bump version for ANY change to layout, field order,
// semantics or texture-reference convention: the runtime rejects a mismatch
// rather than migrating, which is safe because baked files never ship
// independently of the executable.
namespace sponge::scene::asset {

// Eight bytes rather than a packed integer: the tag is five characters, and
// it reads as itself in a hex dump.
constexpr char     magic[8] = { 'S', 'P', 'N', 'G', 'A', 0, 0, 0 };
constexpr uint32_t version  = 1;

// Bumping either of these changes the on-disk vertex layout. The asserts
// fire on a size change or a field reorder; bump version in the same edit.
static_assert(sizeof(Vertex) == 48);
static_assert(offsetof(Vertex, tangent) == 32);

constexpr size_t textureSlots = 5;

struct Header {
    char     magic[sizeof(asset::magic)];
    uint32_t version;
    uint32_t vertexSize;  // sizeof(Vertex) at bake time
    uint32_t meshCount;
    uint32_t textureCount;
};

// Byte offsets are absolute from the start of the file.
struct MeshEntry {
    uint32_t         vertexOffset;
    uint32_t         vertexCount;
    uint32_t         indexOffset;
    uint32_t         indexCount;
    int32_t          textureIndex[textureSlots];  // -1 when the slot is empty
    float            metallicFactor;
    float            roughnessFactor;
    MeshUVTransforms uvTransforms;
};

struct TextureEntry {
    uint32_t offset;
    uint32_t size;
};

// Assembles the bytes of a .spnga file. Each mesh's images must carry KTX2
// bytes; identical names are stored once.
std::vector<uint8_t> write(std::span<const ParsedMesh> meshes);

// Reads a .spnga file into the same ParsedMesh form the glTF importer
// produces, so buildMesh() does not care which path a model came from.
// Returns an empty ModelData on any failure.
ModelData read(const std::string& path);

// Mesh count from the header alone, for progress-bar sizing.
std::size_t readMeshCount(const std::string& path);

}  // namespace sponge::scene::asset
