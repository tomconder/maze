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

// One mesh primitive's worth of CPU-parsed data: vertices/indices plus its
// material images, as indices into ModelData::images.
struct ParsedMesh {
    std::vector<Vertex>     vertices;
    std::vector<uint32_t>   indices;
    std::optional<uint32_t> albedo;
    std::optional<uint32_t> normal;
    std::optional<uint32_t> occlusion;
    std::optional<uint32_t> emissive;
    std::optional<uint32_t> metallicRoughness;
    float                   metallicFactor{ 0.F };
    float                   roughnessFactor{ .5F };
    MeshUVTransforms        uvTransforms;
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
