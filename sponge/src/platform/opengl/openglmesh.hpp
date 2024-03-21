#pragma once

#include "platform/opengl/openglbuffer.hpp"
#include "platform/opengl/openglelementbuffer.hpp"
#include "platform/opengl/opengltexture.hpp"
#include "platform/opengl/openglvertexarray.hpp"
#include "renderer/mesh.hpp"
#include <memory>
#include <string>
#include <vector>

namespace sponge::renderer {

class OpenGLMesh : public Mesh {
   public:
    OpenGLMesh(const std::string& shaderName,
               const std::vector<Vertex>& vertices,
               const std::vector<uint32_t>& indices,
               const std::vector<std::shared_ptr<OpenGLTexture>>& textures);
    void render() const;

   private:
    OpenGLMesh() = default;

    std::string meshShader;

    std::unique_ptr<OpenGLBuffer> vbo;
    std::unique_ptr<OpenGLElementBuffer> ebo;
    std::unique_ptr<OpenGLVertexArray> vao;
    std::vector<std::shared_ptr<OpenGLTexture>> textures;
};

}  // namespace sponge::renderer
