#pragma once

#include "platform/opengl/renderer/indexbuffer.hpp"
#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/renderer/vertexarray.hpp"
#include "platform/opengl/renderer/vertexbuffer.hpp"
#include <memory>
#include <string>

namespace sponge::platform::opengl::scene {

class Grid {
   public:
    Grid();
    void render() const;

    static std::string getShaderName() {
        return std::string(shaderName);
    }

   private:
    static constexpr char shaderName[] = "infinitegrid";

    std::shared_ptr<renderer::Shader> shader;

    std::unique_ptr<renderer::VertexBuffer> vbo;
    std::unique_ptr<renderer::IndexBuffer> ebo;
    std::unique_ptr<renderer::VertexArray> vao;
};

}  // namespace sponge::platform::opengl::scene
