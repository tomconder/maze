#pragma once
#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/renderer/ssbo.hpp"
#include "platform/opengl/renderer/vertexarray.hpp"
#include "platform/opengl/renderer/vertexbuffer.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <span>
#include <string_view>

namespace sponge::platform::opengl::scene {

class Cube {
public:
    // One cube of an instanced draw, std430: three float4, each xyz used. Must
    // match cubeInstances in cube.slang and depthprepass.slang.
    struct Instance {
        glm::vec4 position;
        glm::vec4 prevPosition;
        glm::vec4 color;
    };
    static_assert(sizeof(Instance) == 48);
    // At least ClusteredLights::maxLights; a longer span is cut to this.
    static constexpr int maxInstances = 128;

    Cube();

    // Uploads the per-cube data that renderInstanced() draws.
    void setInstances(std::span<const Instance> instances) const;
    // Draws the first count instances in one call. Binds the instance buffer
    // to SSBO binding 8.
    void renderInstanced(int count) const;

    std::shared_ptr<renderer::Shader> getShader() const {
        return shader;
    }

private:
    static constexpr std::string_view shaderName = "cube";

    std::shared_ptr<renderer::Shader> shader;

    std::unique_ptr<renderer::VertexBuffer> vbo;
    std::unique_ptr<renderer::VertexArray>  vao;
    renderer::SSBO                          instances;
};

}  // namespace sponge::platform::opengl::scene
