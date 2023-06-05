#pragma once

#include "renderer/mesh.h"
#include "renderer/opengl/gl.h"
#include "renderer/opengl/openglbuffer.h"
#include "renderer/opengl/openglelementbuffer.h"
#include "renderer/opengl/opengltexture.h"
#include "renderer/opengl/openglvertexarray.h"

namespace Sponge {

class OpenGLMesh : public Mesh {
   public:
    OpenGLMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices,
               const std::vector<std::shared_ptr<OpenGLTexture>>& textures);
    void render() const;

    std::unique_ptr<OpenGLBuffer> vbo;
    std::unique_ptr<OpenGLElementBuffer> ebo;
    std::unique_ptr<OpenGLVertexArray> vao;
    std::vector<std::shared_ptr<OpenGLTexture>> textures;
};

}  // namespace Sponge
