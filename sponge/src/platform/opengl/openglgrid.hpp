#pragma once

#include "openglbuffer.hpp"
#include "openglelementbuffer.hpp"
#include "openglvertexarray.hpp"
#include <string>

namespace sponge::renderer {

class OpenGLGrid {
   public:
    explicit OpenGLGrid(const std::string& shaderName);
    void render() const;

   private:
    std::string shaderName;

    std::unique_ptr<OpenGLBuffer> vbo;
    std::unique_ptr<OpenGLElementBuffer> ebo;
    std::unique_ptr<OpenGLVertexArray> vao;
};

}  // namespace sponge::renderer