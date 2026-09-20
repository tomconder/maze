#include "platform/opengl/scene/cube.hpp"

#include "logging/log.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"
#include "platform/opengl/scene/unitcube.hpp"

#include <glm/glm.hpp>
#include <cstdint>
#include <memory>

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;

Cube::Cube() {
    const auto shaderCreateInfo = renderer::ShaderCreateInfo{
        .name           = shaderName.data(),
        .vertexShader   = "cube.vert",
        .fragmentShader = "cube.frag",
    };
    shader = AssetManager::createShader(shaderCreateInfo);
    shader->bind();

    vao = std::make_unique<renderer::VertexArray>();
    vao->bind();

    vbo = std::make_unique<renderer::VertexBuffer>(unitCubeVertices.data(),
                                                   sizeof(unitCubeVertices));
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
    glDrawArrays(GL_TRIANGLES, 0, unitCubeVertexCount);
    vao->unbind();
}

}  // namespace sponge::platform::opengl::scene
