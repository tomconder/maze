#include "platform/opengl/scene/quad.hpp"

#include "platform/opengl/renderer/assetmanager.hpp"

#include <array>
#include <memory>

namespace {
constexpr std::array<uint32_t, 6> indices = {
    0, 2, 1,  //
    0, 3, 2   //
};
constexpr uint32_t indexCount  = 6;
constexpr uint32_t vertexCount = 4;
}  // namespace

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;

std::shared_ptr<renderer::Shader> Quad::shader;

Quad::Quad() {
    const auto shaderCreateInfo = renderer::ShaderCreateInfo{
        .name               = shaderName.data(),
        .vertexShaderPath   = "/shaders/glsl/quad.vert.glsl",
        .fragmentShaderPath = "/shaders/glsl/quad.frag.glsl",
    };
    shader = AssetManager::createShader(shaderCreateInfo);
    shader->bind();

    vao = renderer::VertexArray::create();
    vao->bind();

    vbo = std::make_unique<renderer::VertexBuffer>(
        nullptr, vertexCount * sizeof(glm::vec2));
    vbo->bind();

    ebo = std::make_unique<renderer::IndexBuffer>(indices.data(),
                                                  sizeof(indices));
    ebo->bind();

    constexpr uint32_t kPositionLoc = 0;
    glEnableVertexAttribArray(kPositionLoc);
    glVertexAttribPointer(kPositionLoc, 2, GL_FLOAT, GL_FALSE,
                          2 * sizeof(GLfloat),
                          reinterpret_cast<const void*>(0));

    vbo->unbind();
    vao->unbind();

    shader->unbind();
}

void Quad::render(const glm::vec2& top, const glm::vec2& bottom,
                  const glm::vec4& color, const float cornerRadius,
                  const float borderWidth, const glm::vec4& borderColor) const {
    const std::array<glm::vec2, vertexCount> vertices{ {
        { top.x, bottom.y },    //
        { top.x, top.y },       //
        { bottom.x, top.y },    //
        { bottom.x, bottom.y }  //
    } };

    const auto corners = glm::vec4(top.x, top.y, bottom.x, bottom.y);

    vao->bind();

    shader->bind();
    shader->setFloat4("color", color);
    shader->setFloat4("corners", corners);
    shader->setFloat("cornerRadius", cornerRadius);
    shader->setFloat("borderWidth", borderWidth);
    shader->setFloat4("borderColor", borderColor);

    vbo->update(vertices.data(), vertexCount * sizeof(glm::vec2));

    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);

    shader->unbind();

    vao->unbind();
}

}  // namespace sponge::platform::opengl::scene
