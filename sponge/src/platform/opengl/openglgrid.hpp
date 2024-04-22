#pragma once

#include "openglindexbuffer.hpp"
#include "openglvertexarray.hpp"
#include "openglvertexbuffer.hpp"
#include <string>

namespace sponge::platform::opengl {

class OpenGLGrid {
   public:
    explicit OpenGLGrid(const std::string& shaderName);
    void render() const;

   private:
    std::string shaderName;

    std::unique_ptr<OpenGLVertexBuffer> vbo;
    std::unique_ptr<OpenGLIndexBuffer> ebo;
    std::unique_ptr<OpenGLVertexArray> vao;
};

}  // namespace sponge::renderer
