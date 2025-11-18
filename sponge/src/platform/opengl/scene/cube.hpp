#pragma once
#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/renderer/vertexarray.hpp"
#include "platform/opengl/renderer/vertexbuffer.hpp"

#include <memory>
#include <string>
#include <string_view>

namespace sponge::platform::opengl::scene {

class Cube {
public:
    Cube();
    void render() const;

    static std::string_view getShaderName() {
        return shaderName;
    }

private:
    static const std::string shaderName;

    std::shared_ptr<renderer::Shader> shader;

    std::unique_ptr<renderer::VertexBuffer> vbo;
    std::unique_ptr<renderer::VertexArray>  vao;
};

}  // namespace sponge::platform::opengl::scene
