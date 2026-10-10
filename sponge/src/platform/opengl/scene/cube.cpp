#include "platform/opengl/scene/cube.hpp"

#include "logging/log.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"
#include "platform/opengl/scene/unitcube.hpp"

#include <glm/glm.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;

Cube::Cube() :
    instances(static_cast<std::size_t>(maxInstances) * sizeof(Instance)) {
    const auto shaderCreateInfo = renderer::ShaderCreateInfo{
        .name           = shaderName.data(),
        .vertexShader   = "cube.vert",
        .fragmentShader = "cube.frag",
    };
    shader = AssetManager::createShader(shaderCreateInfo);
    vao    = std::make_unique<renderer::VertexArray>();
    vbo    = std::make_unique<renderer::VertexBuffer>(unitCubeVertices.data(),
                                                      sizeof(unitCubeVertices));
    vao->setVertexBuffer(*vbo, sizeof(glm::vec3));

    constexpr uint32_t positionLoc = 0;
    vao->addAttribute(positionLoc, 3, 0);
}

void Cube::setInstances(const std::span<const Instance> data) const {
    const auto count = std::min(data.size(), std::size_t{ maxInstances });
    instances.update(data.data(), count * sizeof(Instance));
}

void Cube::renderInstanced(const int count) const {
    instances.bindBase(8);
    vao->bind();
    glDrawArraysInstanced(GL_TRIANGLES, 0, unitCubeVertexCount,
                          std::min(count, maxInstances));
    vao->unbind();
}

}  // namespace sponge::platform::opengl::scene
