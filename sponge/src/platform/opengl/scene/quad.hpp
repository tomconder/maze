#pragma once

#include "platform/opengl/renderer/indexbuffer.hpp"
#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/renderer/vertexarray.hpp"
#include "platform/opengl/renderer/vertexbuffer.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <string_view>

namespace sponge::platform::opengl::scene {

class Quad {
public:
    Quad();
    void render(const glm::vec2& top, const glm::vec2& bottom,
                const glm::vec4& color, float cornerRadius = 0.0f,
                float            borderWidth = 0.0f,
                const glm::vec4& borderColor = glm::vec4(0.0f)) const;

    static std::string_view getShaderName() {
        return shaderName;
    }

private:
    static constexpr std::string_view shaderName = "quad";

    std::shared_ptr<renderer::Shader> shader;

    std::unique_ptr<renderer::VertexBuffer> vbo;
    std::unique_ptr<renderer::IndexBuffer>  ebo;
    std::unique_ptr<renderer::VertexArray>  vao;
};

}  // namespace sponge::platform::opengl::scene
