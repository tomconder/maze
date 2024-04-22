#pragma once

#include "platform/opengl/openglindexbuffer.hpp"
#include "platform/opengl/opengltexture.hpp"
#include "platform/opengl/openglvertexarray.hpp"
#include "platform/opengl/openglvertexbuffer.hpp"
#include "renderer/mesh.hpp"
#include <memory>
#include <string>
#include <vector>

namespace sponge::platform::opengl {

class OpenGLMesh : public renderer::Mesh {
   public:
    OpenGLMesh(const std::string& shaderName,
               const std::vector<renderer::Vertex>& vertices,
               const std::vector<uint32_t>& indices,
               const std::vector<std::shared_ptr<OpenGLTexture>>& textures);
    void render() const;

   private:
    std::string shaderName;

    std::unique_ptr<OpenGLVertexBuffer> vbo;
    std::unique_ptr<OpenGLIndexBuffer> ebo;
    std::unique_ptr<OpenGLVertexArray> vao;
    std::vector<std::shared_ptr<OpenGLTexture>> textures;
};

}  // namespace sponge::platform::opengl
