#include "platform/opengl/scene/cube.hpp"

#include "logging/log.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"

#include <array>
#include <memory>

namespace {
constexpr std::array vertices = {
    glm::vec3{ -0.5, 0.5, -0.5 },  glm::vec3{ -0.5, 0.5, 0.5 },
    glm::vec3{ 0.5, 0.5, 0.5 },    glm::vec3{ -0.5, 0.5, -0.5 },
    glm::vec3{ 0.5, 0.5, 0.5 },    glm::vec3{ 0.5, 0.5, -0.5 },
    glm::vec3{ -0.5, 0.5, -0.5 },  glm::vec3{ -0.5, -0.5, -0.5 },
    glm::vec3{ -0.5, -0.5, 0.5 },  glm::vec3{ -0.5, 0.5, -0.5 },
    glm::vec3{ -0.5, -0.5, 0.5 },  glm::vec3{ -0.5, 0.5, 0.5 },
    glm::vec3{ 0.5, 0.5, 0.5 },    glm::vec3{ 0.5, -0.5, 0.5 },
    glm::vec3{ 0.5, -0.5, -0.5 },  glm::vec3{ 0.5, 0.5, 0.5 },
    glm::vec3{ 0.5, -0.5, -0.5 },  glm::vec3{ 0.5, 0.5, -0.5 },
    glm::vec3{ 0.5, 0.5, -0.5 },   glm::vec3{ 0.5, -0.5, -0.5 },
    glm::vec3{ -0.5, -0.5, -0.5 }, glm::vec3{ 0.5, 0.5, -0.5 },
    glm::vec3{ -0.5, -0.5, -0.5 }, glm::vec3{ -0.5, 0.5, -0.5 },
    glm::vec3{ -0.5, 0.5, 0.5 },   glm::vec3{ -0.5, -0.5, 0.5 },
    glm::vec3{ 0.5, -0.5, 0.5 },   glm::vec3{ -0.5, 0.5, 0.5 },
    glm::vec3{ 0.5, -0.5, 0.5 },   glm::vec3{ 0.5, 0.5, 0.5 },
    glm::vec3{ -0.5, -0.5, 0.5 },  glm::vec3{ -0.5, -0.5, -0.5 },
    glm::vec3{ 0.5, -0.5, -0.5 },  glm::vec3{ -0.5, -0.5, 0.5 },
    glm::vec3{ 0.5, -0.5, -0.5 },  glm::vec3{ 0.5, -0.5, 0.5 }
};
constexpr uint32_t vertexCount = 36;
}  // namespace

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;

Cube::Cube() {
    const auto shaderCreateInfo = renderer::ShaderCreateInfo{
        .name               = shaderName.data(),
        .vertexShaderPath   = "/shaders/glsl/cube.vert.glsl",
        .fragmentShaderPath = "/shaders/glsl/cube.frag.glsl"
    };
    shader = AssetManager::createShader(shaderCreateInfo);
    shader->bind();

    vao = renderer::VertexArray::create();
    vao->bind();

    vbo = std::make_unique<renderer::VertexBuffer>(vertices.data(),
                                                   sizeof(vertices));
    vbo->bind();

    constexpr uint32_t positionLoc = 0;
    glEnableVertexAttribArray(positionLoc);
    glVertexAttribPointer(positionLoc, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3),
                          reinterpret_cast<const void*>(0));

    shader->unbind();
    vao->unbind();
}

void Cube::render() const {
    vao->bind();
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    vao->unbind();
}

}  // namespace sponge::platform::opengl::scene
