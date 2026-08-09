#pragma once

#include "platform/opengl/renderer/texture.hpp"
#include "platform/opengl/scene/mesh.hpp"
#include "scene/mesh.hpp"

#include <glm/glm.hpp>
#include <cgltf.h>
#include <tiny_obj_loader.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace sponge::platform::opengl::scene {

struct ModelCreateInfo {
    std::string name;
    std::string path;
    std::string assetsFolder = core::File::getResourceDir();
};

// Decoded (CPU-side) image: raw pixels, no GL texture yet. Produced by
// Model::parse() — safe to build on any thread. Turned into a Texture only
// by Model::build(), which must run on the GL thread.
struct ParsedImage {
    std::string          name;
    uint32_t             width{ 0 };
    uint32_t             height{ 0 };
    uint32_t             bytesPerPixel{ 0 };
    std::vector<uint8_t> pixels;
};

// One mesh primitive's worth of CPU-parsed data: vertices/indices plus
// decoded (not yet GL-uploaded) material images.
struct ParsedMesh {
    std::vector<sponge::scene::Vertex> vertices;
    std::vector<uint32_t>              indices;
    std::optional<ParsedImage>         albedo;
    std::optional<ParsedImage>         normal;
    std::optional<ParsedImage>         occlusion;
    std::optional<ParsedImage>         emissive;
    std::optional<ParsedImage>         metallicRoughness;
    float                              metallicFactor{ 0.F };
    float                              roughnessFactor{ .5F };
    MeshUVTransforms                   uvTransforms;
};

// CPU-only parse result for a whole model. Safe to build on any thread;
// Model::build() turns it into GL objects on the GL thread.
struct ModelData {
    std::vector<ParsedMesh> meshes;
};

class Model {
public:
    // Eager, synchronous load: parse() + buildMesh() per mesh, on the
    // calling thread. Must be called on the GL thread.
    explicit Model(const ModelCreateInfo& createInfo);

    // Assembles a model from meshes already built (via buildMesh(), e.g. one
    // per frame on the GL thread). Must be called on the GL thread.
    explicit Model(std::vector<std::shared_ptr<Mesh>>&& builtMeshes);

    // CPU-only parse, no GL/AssetManager touch — safe on any thread.
    // onMeshParsed, if set, fires once per mesh appended to the result.
    static ModelData parse(const ModelCreateInfo&       createInfo,
                           const std::function<void()>& onMeshParsed = {});

    // Structural mesh count without decoding data, for progress-bar sizing.
    // glTF counts accurately; OBJ has no cheap count in tinyobjloader, so
    // it always returns 1 (fine — no .obj assets today).
    static std::size_t countMeshes(const ModelCreateInfo& createInfo);

    // Builds one mesh's GL objects from CPU-parsed data. GL thread only —
    // exposed so a caller can spread a many-mesh model's upload across frames.
    static std::shared_ptr<Mesh> buildMesh(ParsedMesh&& parsedMesh);
    static std::shared_ptr<renderer::Texture>
        buildTexture(std::optional<ParsedImage>&& image);

    void   render(const std::shared_ptr<renderer::Shader>& shader) const;
    size_t getNumIndices() const {
        return numIndices;
    }
    size_t getNumVertices() const {
        return numVertices;
    }

protected:
    std::vector<std::shared_ptr<Mesh>> meshes;

private:
    size_t numIndices  = 0;
    size_t numVertices = 0;

    static ModelData parseObj(const std::string&           path,
                              const std::function<void()>& onMeshParsed);
    static ParsedMesh
        parseObjMesh(tinyobj::attrib_t& attrib, tinyobj::mesh_t& mesh,
                     const std::vector<tinyobj::material_t>& materials,
                     const std::string&                      path);
    static std::optional<ParsedImage>
        decodeMaterialTexture(const tinyobj::material_t& material,
                              const std::string&         path);

    static ModelData   parseGltf(const std::string&           path,
                                 const std::function<void()>& onMeshParsed);
    static std::size_t countGltfMeshes(const std::string& path);
    static std::optional<ParsedMesh>
        parseGltfPrimitive(const cgltf_primitive& primitive,
                           const glm::mat4& transform, const std::string& path);
    static void computeTangents(std::vector<sponge::scene::Vertex>& vertices,
                                const std::vector<uint32_t>&        indices);
    static std::optional<ParsedImage>
                       decodeGltfTexture(const cgltf_texture_view& textureView,
                                         const std::string&        path);
    static UVTransform gltfUVTransform(const cgltf_texture_view& textureView);
};

}  // namespace sponge::platform::opengl::scene
