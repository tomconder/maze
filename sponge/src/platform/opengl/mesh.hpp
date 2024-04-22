#pragma once

#include "platform/opengl/indexbuffer.hpp"
#include "platform/opengl/texture.hpp"
#include "platform/opengl/vertexarray.hpp"
#include "platform/opengl/vertexbuffer.hpp"
#include "renderer/mesh.hpp"
#include <memory>
#include <string>
#include <vector>

namespace sponge::platform::opengl {

class Mesh : public renderer::Mesh {
   public:
    Mesh(const std::string& shaderName,
         const std::vector<renderer::Vertex>& vertices,
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

}  // namespace sponge::platform::opengl
