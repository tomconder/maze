#pragma once
#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/renderer/vertexarray.hpp"
#include "platform/opengl/renderer/vertexbuffer.hpp"

#include <memory>
#include <string_view>

namespace sponge::platform::opengl::scene {

class Cube {
public:
    Cube();
    void render() const;

    std::shared_ptr<renderer::Shader> getShader() const {
        return shader;
    }

private:
    static constexpr std::string_view shaderName = "cube";

    std::shared_ptr<renderer::Shader> shader;

    std::unique_ptr<renderer::VertexBuffer> vbo;
    std::unique_ptr<renderer::VertexArray>  vao;
};

}  // namespace sponge::platform::opengl::scene
