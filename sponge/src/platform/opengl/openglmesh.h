#pragma once

#include "graphics/renderer/mesh.h"
#include "platform/opengl/gl.h"
#include "platform/opengl/openglbuffer.h"
#include "platform/opengl/openglelementbuffer.h"
#include "platform/opengl/opengltexture.h"
#include "platform/opengl/openglvertexarray.h"

namespace sponge::graphics::renderer {

class OpenGLMesh : public Mesh {
   public:
    OpenGLMesh(const std::vector<Vertex>& vertices,
               const std::vector<uint32_t>& indices,
               const std::vector<std::shared_ptr<OpenGLTexture>>& textures);
    void render() const;

    std::unique_ptr<OpenGLBuffer> vbo;
    std::unique_ptr<OpenGLElementBuffer> ebo;
    std::unique_ptr<OpenGLVertexArray> vao;
    std::vector<std::shared_ptr<OpenGLTexture>> textures;
};

}  // namespace sponge::graphics::renderer