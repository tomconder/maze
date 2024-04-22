#pragma once

#include "indexbuffer.hpp"
#include "vertexarray.hpp"
#include "vertexbuffer.hpp"
#include <string>

namespace sponge::platform::opengl {

class Grid {
   public:
    explicit Grid(const std::string& shaderName);
    void render() const;

   private:
    std::string shaderName;

    std::unique_ptr<VertexBuffer> vbo;
    std::unique_ptr<IndexBuffer> ebo;
    std::unique_ptr<VertexArray> vao;
};

}  // namespace sponge::platform::opengl
