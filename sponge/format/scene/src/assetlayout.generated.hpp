// Generated from tools/yeschef/format by cbindgen. Do not edit.
// To change the layout, edit the Rust source and run
//   UPDATE_LAYOUT_HEADER=1 cargo test
// in tools/yeschef.

#pragma once

#include <cstdint>
#include <cstddef>

namespace sponge::scene::layout {

/// Number of texture slots in a mesh entry. The order is albedo, normal,
/// occlusion, emissive, metallic-roughness, diffuse transmission.
constexpr static const size_t TEXTURE_SLOTS = 6;

/// Model container version.
constexpr static const uint32_t ASSET_VERSION = 9;

/// Model container magic: the bytes `SPNGA\0\0\0`, read as a little endian
/// integer.
constexpr static const uint64_t ASSET_MAGIC = 280369188947;

/// Shader pack version.
constexpr static const uint32_t SHADERPACK_VERSION = 1;

/// Shader pack magic: the bytes `SPNGSH\0\0`, read as a little endian
/// integer.
constexpr static const uint64_t SHADERPACK_MAGIC = 79522515800147;

/// A sphere and the simplification error inside it, in model units. The error
/// is how far the simplified surface can be from LOD0.
struct ClusterBounds {
  float center[3];
  float radius;
  float error;
};

/// One cluster of a mesh's cluster LOD hierarchy: a range of the mesh indices.
/// A view draws a cluster when its error test passes for `bounds` and fails
/// for `parent`. `bounds.error` is 0 for a LOD0 cluster. `parent.error` is
/// `FLT_MAX` for a cluster with no coarser version.
struct MeshCluster {
  uint32_t firstIndex;
  uint32_t indexCount;
  /// 0 is LOD0.
  uint32_t depth;
  ClusterBounds bounds;
  ClusterBounds parent;
};

/// What a baked model stores and what the GPU reads. `normal` and `tangent`
/// are signed normalized 10:10:10:2 integers (`GL_INT_2_10_10_10_REV`).
/// Positions and UVs stay float.
struct PackedVertex {
  float position[3];
  float texCoords[2];
  /// xyz; w unused.
  uint32_t normal;
  /// xyz; w is the bitangent sign, -1 or 1.
  uint32_t tangent;
};

/// KHR_texture_transform offset and scale. Rotation is not supported.
struct UvTransform {
  float offset[2];
  float scale[2];
};

struct MeshUvTransforms {
  UvTransform albedo;
  UvTransform normal;
  UvTransform occlusion;
  UvTransform emissive;
  UvTransform metallicRoughness;
  UvTransform diffuseTransmission;
};

/// Start of a model container.
struct AssetHeader {
  /// The bytes of `ASSET_MAGIC`.
  uint8_t magic[8];
  uint32_t version;
  /// `size_of::<PackedVertex>()` at bake time.
  uint32_t vertexSize;
  uint32_t meshCount;
  uint32_t textureCount;
};

/// One mesh. The `mesh_count` entries follow the header. Vertex, index and
/// cluster blobs start on a 16-byte boundary.
struct MeshEntry {
  uint32_t vertexOffset;
  uint32_t vertexCount;
  uint32_t indexOffset;
  uint32_t indexCount;
  /// `MeshCluster[cluster_count]`.
  uint32_t clusterOffset;
  /// At least 1.
  uint32_t clusterCount;
  /// Index into the texture table, or -1 when the slot is empty.
  int32_t textureIndex[TEXTURE_SLOTS];
  float metallicFactor;
  float roughnessFactor;
  float clearcoatFactor;
  float clearcoatRoughnessFactor;
  /// 0 opaque, 1 mask, 2 blend.
  uint32_t alphaMode;
  /// glTF MASK cutoff, else 0.
  float alphaCutoff;
  /// 0 or 1.
  uint32_t doubleSided;
  /// Linear RGBA.
  float baseColorFactor[4];
  /// rgb is the color, w the factor.
  float diffuseTransmission[4];
  MeshUvTransforms uvTransforms;
};

/// One KTX2 file stored whole. The `texture_count` entries follow the mesh
/// entries.
struct TextureEntry {
  uint32_t offset;
  uint32_t size;
};

/// Start of a shader pack.
struct ShaderPackHeader {
  /// The bytes of `SHADERPACK_MAGIC`.
  uint8_t magic[8];
  uint32_t version;
  uint32_t count;
};

/// One shader. The `count` entries follow the header, sorted by name.
struct ShaderPackEntry {
  uint32_t nameOffset;
  uint32_t nameSize;
  uint32_t sourceOffset;
  uint32_t sourceSize;
};

}  // namespace sponge::scene::layout
