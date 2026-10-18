#pragma once

#include "vertex.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace sponge::scene {

// KHR_texture_transform offset/scale (no rotation support; unused by our
// current gltf assets/gltfpack output).
struct UVTransform {
    glm::vec2 offset{ 0.F, 0.F };
    glm::vec2 scale{ 1.F, 1.F };
};

struct MeshUVTransforms {
    UVTransform albedo;
    UVTransform normal;
    UVTransform occlusion;
    UVTransform emissive;
    UVTransform metallicRoughness;
    UVTransform diffuseTransmission;
};

// A CPU-side texture, no GL object yet. Exactly one of pixels or ktx2 is
// populated: the glTF importer decodes to pixels, a baked .spnga carries
// whole KTX2 files. Safe to build on any thread.
struct ParsedImage {
    std::string          name;
    uint32_t             width{ 0 };
    uint32_t             height{ 0 };
    uint32_t             bytesPerPixel{ 0 };
    std::vector<uint8_t> pixels;
    std::vector<uint8_t> ktx2;
};

// glTF alphaMode. Mask discards below ParsedMesh::alphaCutoff. Blend is
// carried but not drawn yet, so it renders as Opaque.
enum class AlphaMode : uint32_t { Opaque, Mask, Blend };

// A sphere and the simplification error inside it, in model units. The
// error is how far the simplified surface can be from LOD0.
struct ClusterBounds {
    glm::vec3 center{ 0.F };
    float     radius{ 0.F };
    float     error{ 0.F };

    bool operator==(const ClusterBounds&) const = default;
};

// One cluster (up to 128 triangles) of a mesh's cluster LOD hierarchy: a
// range of ParsedMesh::indices. A view draws a cluster when its error test
// passes for self and fails for parent. self.error is 0 for a LOD0
// cluster. parent.error is FLT_MAX for a cluster that has no coarser
// version.
struct MeshCluster {
    uint32_t      firstIndex{ 0 };
    uint32_t      indexCount{ 0 };
    uint32_t      depth{ 0 };  // 0 is LOD0
    ClusterBounds self;
    ClusterBounds parent;

    bool operator==(const MeshCluster&) const = default;
};

// One mesh primitive's worth of CPU-parsed data: vertices/indices plus its
// material images, as indices into ModelData::images.
struct ParsedMesh {
    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;
    // The cluster LOD hierarchy over indices. The LOD0 clusters come first,
    // so indices starts with all of LOD0. Empty means one LOD0 range over all
    // of indices: the glTF importer leaves it empty, and the bake and the
    // reader fill it.
    std::vector<MeshCluster> clusters;
    std::optional<uint32_t>  albedo;
    std::optional<uint32_t>  normal;
    std::optional<uint32_t>  occlusion;
    std::optional<uint32_t>  emissive;
    std::optional<uint32_t>  metallicRoughness;
    // KHR_materials_diffuse_transmission strength map; its alpha scales the
    // factor below.
    std::optional<uint32_t> diffuseTransmissionMap;
    // glTF baseColorFactor, linear, multiplied into the albedo texture. Its
    // alpha scales the albedo alpha for the MASK test below. Without a
    // texture it is the albedo.
    glm::vec4 baseColorFactor{ 1.F };
    float     metallicFactor{ 0.F };
    float     roughnessFactor{ .5F };
    // glTF alphaMode. alphaCutoff only means something for Mask: a fragment
    // whose albedo alpha is below it is discarded. It is 0 for the other
    // modes.
    AlphaMode alphaMode{ AlphaMode::Opaque };
    float     alphaCutoff{ 0.F };
    // glTF doubleSided: both faces are drawn, and the normal of a back face is
    // flipped.
    bool doubleSided{ false };
    // KHR_materials_clearcoat factors; both default to 0 (no coat). The coat
    // textures are not carried.
    float clearcoatFactor{ 0.F };
    float clearcoatRoughnessFactor{ 0.F };
    // KHR_materials_diffuse_transmission: rgb = color, w = factor. A factor of
    // 0 means opaque. The color texture is not carried.
    glm::vec4        diffuseTransmission{ 1.F, 1.F, 1.F, 0.F };
    MeshUVTransforms uvTransforms;
};

// CPU-only parse result for a whole model. Model::buildMesh() turns it into
// GL objects on the GL thread. Materials share images, so each image is held
// once and meshes refer to it by index: sponza has 307 texture slots over 69
// images.
struct ModelData {
    std::vector<ParsedImage> images;
    std::vector<ParsedMesh>  meshes;
};

}  // namespace sponge::scene
