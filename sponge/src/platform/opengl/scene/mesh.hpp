#pragma once

#include "platform/opengl/renderer/indexbuffer.hpp"
#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/renderer/texture.hpp"
#include "platform/opengl/renderer/vertexarray.hpp"
#include "platform/opengl/renderer/vertexbuffer.hpp"
#include "scene/mesh.hpp"
#include <memory>
#include <string>
#include <vector>

namespace sponge::platform::opengl::scene {

class Mesh : public sponge::scene::Mesh {
   public:
    Mesh(const std::vector<sponge::scene::Vertex>& vertices,
         const std::vector<uint32_t>& indices,
         const std::vector<std::shared_ptr<renderer::Texture>>& textures);
    void render() const;

    static std::string getShaderName() {
        return std::string(shaderName);
    }

   private:
    static constexpr char shaderName[] = "mesh";
    int32_t indexCount = 0;

    std::shared_ptr<renderer::Shader> shader;

    std::unique_ptr<renderer::VertexBuffer> vbo;
    std::unique_ptr<renderer::IndexBuffer> ebo;
    std::unique_ptr<renderer::VertexArray> vao;

    std::vector<std::shared_ptr<renderer::Texture>> textures;
};

}  // namespace sponge::platform::opengl::scene
