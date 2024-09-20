#pragma once

#include "platform/opengl/renderer/indexbuffer.hpp"
#include "platform/opengl/renderer/texture.hpp"
#include "platform/opengl/renderer/vertexarray.hpp"
#include "platform/opengl/renderer/vertexbuffer.hpp"
#include "renderer/mesh.hpp"
#include <memory>
#include <string>
#include <vector>

namespace sponge::platform::opengl::renderer {

class Mesh : public sponge::renderer::Mesh {
   public:
    Mesh(const std::string& shaderName,
         const std::vector<sponge::renderer::Vertex>& vertices,
         const std::vector<uint32_t>& indices,
         const std::vector<std::shared_ptr<Texture>>& textures);
    void render() const;

   private:
    std::string shaderName;

    std::unique_ptr<VertexBuffer> vbo;
    std::unique_ptr<IndexBuffer> ebo;
    std::unique_ptr<VertexArray> vao;
    std::vector<std::shared_ptr<Texture>> textures;
};

}  // namespace sponge::platform::opengl::renderer
