#pragma once

#include "assetlayout.generated.hpp"
#include "modeldata.hpp"
#include "vertex.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Baked model container (.spnga). Written by tools/yeschef, read by
// Model::parse(). One fread, then views over the buffer.
//
// The byte layout is defined in Rust, in tools/yeschef/format, and reaches
// this code as assetlayout.generated.hpp. Edit the Rust source, not that
// header. magic and version are the first two fields and stay at offsets 0
// and 8 forever, so any future revision can identify any file before trusting
// a single other byte. The runtime rejects a version mismatch rather than
// migrating, which is safe because baked files never ship independently of the
// executable.
namespace sponge::scene::asset {

static_assert(std::endian::native == std::endian::little);

using Header       = layout::AssetHeader;
using MeshEntry    = layout::MeshEntry;
using TextureEntry = layout::TextureEntry;

// The magic is eight bytes rather than a packed integer in the file: the tag
// is five characters, and it reads as itself in a hex dump.
constexpr auto magic =
    std::bit_cast<std::array<uint8_t, 8>>(layout::ASSET_MAGIC);
constexpr uint32_t version      = layout::ASSET_VERSION;
constexpr size_t   textureSlots = layout::TEXTURE_SLOTS;

// write() and read() copy these types to and from the file as raw bytes, so
// each must match its twin in the generated header, field by field. glm types
// have no cbindgen form, which is why the twins exist.
static_assert(sizeof(PackedVertex) == sizeof(layout::PackedVertex));
static_assert(offsetof(PackedVertex, position) ==
              offsetof(layout::PackedVertex, position));
static_assert(offsetof(PackedVertex, texCoords) ==
              offsetof(layout::PackedVertex, texCoords));
static_assert(offsetof(PackedVertex, normal) ==
              offsetof(layout::PackedVertex, normal));
static_assert(offsetof(PackedVertex, tangent) ==
              offsetof(layout::PackedVertex, tangent));

static_assert(sizeof(ClusterBounds) == sizeof(layout::ClusterBounds));
static_assert(offsetof(ClusterBounds, center) ==
              offsetof(layout::ClusterBounds, center));
static_assert(offsetof(ClusterBounds, radius) ==
              offsetof(layout::ClusterBounds, radius));
static_assert(offsetof(ClusterBounds, error) ==
              offsetof(layout::ClusterBounds, error));

static_assert(sizeof(MeshCluster) == sizeof(layout::MeshCluster));
static_assert(offsetof(MeshCluster, firstIndex) ==
              offsetof(layout::MeshCluster, firstIndex));
static_assert(offsetof(MeshCluster, indexCount) ==
              offsetof(layout::MeshCluster, indexCount));
static_assert(offsetof(MeshCluster, depth) ==
              offsetof(layout::MeshCluster, depth));
static_assert(offsetof(MeshCluster, self) ==
              offsetof(layout::MeshCluster, bounds));
static_assert(offsetof(MeshCluster, parent) ==
              offsetof(layout::MeshCluster, parent));

static_assert(sizeof(UVTransform) == sizeof(layout::UvTransform));
static_assert(offsetof(UVTransform, offset) ==
              offsetof(layout::UvTransform, offset));
static_assert(offsetof(UVTransform, scale) ==
              offsetof(layout::UvTransform, scale));

static_assert(sizeof(MeshUVTransforms) == sizeof(layout::MeshUvTransforms));
static_assert(offsetof(MeshUVTransforms, albedo) ==
              offsetof(layout::MeshUvTransforms, albedo));
static_assert(offsetof(MeshUVTransforms, normal) ==
              offsetof(layout::MeshUvTransforms, normal));
static_assert(offsetof(MeshUVTransforms, occlusion) ==
              offsetof(layout::MeshUvTransforms, occlusion));
static_assert(offsetof(MeshUVTransforms, emissive) ==
              offsetof(layout::MeshUvTransforms, emissive));
static_assert(offsetof(MeshUVTransforms, metallicRoughness) ==
              offsetof(layout::MeshUvTransforms, metallicRoughness));
static_assert(offsetof(MeshUVTransforms, diffuseTransmission) ==
              offsetof(layout::MeshUvTransforms, diffuseTransmission));

static_assert(sizeof(glm::vec4) == sizeof(float[4]));

// Reads a .spnga file into the same ModelData form the importers produce, so
// buildMesh() does not care which path a model came from.
// On failure, returns an empty ModelData and sets error. error is left
// alone on success, so pass an empty string.
ModelData read(const std::string& path, std::string& error);

// Mesh count from the header alone, for progress-bar sizing. On failure,
// returns 0 and sets error.
std::size_t readMeshCount(const std::string& path, std::string& error);

}  // namespace sponge::scene::asset
