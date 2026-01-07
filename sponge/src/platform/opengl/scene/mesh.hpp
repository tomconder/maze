#pragma once

#include "platform/opengl/renderer/indexbuffer.hpp"
#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/renderer/texture.hpp"
#include "platform/opengl/renderer/vertexarray.hpp"
#include "platform/opengl/renderer/vertexbuffer.hpp"
#include "scene/mesh.hpp"

#include <memory>
#include <string_view>
#include <vector>

namespace sponge::platform::opengl::scene {

class Mesh : public sponge::scene::Mesh {
public:
    Mesh(std::vector<sponge::scene::Vertex>&& vertices, std::size_t numVertices,
         std::vector<uint32_t>&& indices, std::size_t numIndices,
         std::vector<std::shared_ptr<renderer::Texture>>&& textures);
    void render(const std::shared_ptr<renderer::Shader>& shader) const;

    static std::string_view getShaderName() {
        return shaderName;
    }

private:
    static constexpr std::string_view shaderName = "mesh";

    std::unique_ptr<renderer::VertexBuffer> vbo;
    std::unique_ptr<renderer::IndexBuffer>  ebo;
    std::unique_ptr<renderer::VertexArray>  vao;

    std::vector<std::shared_ptr<renderer::Texture>> textures;
};

}  // namespace sponge::platform::opengl::scene
