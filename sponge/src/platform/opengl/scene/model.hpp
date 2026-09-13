#pragma once

#include "platform/opengl/renderer/texture.hpp"
#include "platform/opengl/scene/mesh.hpp"
#include "scene/mesh.hpp"
#include "scene/modeldata.hpp"

#include <glm/glm.hpp>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace sponge::platform::opengl::scene {

using sponge::scene::ModelData;
using sponge::scene::ParsedImage;
using sponge::scene::ParsedMesh;

struct ModelCreateInfo {
    std::string name;
    std::string path;
    std::string assetsFolder = core::File::getResourceDir();
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
    static ModelData parse(const ModelCreateInfo& createInfo);

    // Structural mesh count without decoding data, for progress-bar sizing.
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
};

}  // namespace sponge::platform::opengl::scene
