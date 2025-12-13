#include "platform/opengl/scene/sprite.hpp"

#include "platform/opengl/renderer/assetmanager.hpp"

#include <glm/glm.hpp>

#include <array>
#include <memory>
#include <string>

namespace {
inline const std::string          vertex  = "vertex";
constexpr std::array<uint32_t, 6> indices = {
    0, 1, 2,  //
    0, 2, 3   //
};
constexpr uint32_t indexCount  = 6;
constexpr uint32_t vertexCount = 8;
}  // namespace

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;

inline const std::string Sprite::shaderName = "sprite";

Sprite::Sprite(const std::string& name, const std::string& texturePath) {
    const auto shaderCreateInfo = renderer::ShaderCreateInfo{
        .name               = shaderName,
        .vertexShaderPath   = "/shaders/sprite.vert.glsl",
        .fragmentShaderPath = "/shaders/sprite.frag.glsl",
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

    const auto program = shader->getId();

    if (const auto location = glGetAttribLocation(program, vertex.c_str());
        location != -1) {
        const auto position = static_cast<uint32_t>(location);
        glEnableVertexAttribArray(position);
        glVertexAttribPointer(position, 4, GL_FLOAT, GL_FALSE,
                              4 * sizeof(float), nullptr);
    }

    vbo->unbind();
    vao->unbind();

    const renderer::TextureCreateInfo textureCreateInfo{ .name = name,
                                                         .path = texturePath };
    tex = AssetManager::createTexture(textureCreateInfo);
    tex->bind();

    shader->unbind();
}

void Sprite::render(const glm::vec2& position, const glm::vec2& size) const {
    const std::array<glm::vec2, vertexCount> vertices{
        { { position.x + size.x, position.y },
          { 1.F, 0.F },  //
          { position.x, position.y },
          { 0.F, 0.F },  //
          { position.x, position.y + size.y },
          { 0.F, 1.F },  //
          { position.x + size.x, position.y + size.y },
          { 1.F, 1.F } }
    };

    vao->bind();

    shader->bind();

    tex->bind();

    vbo->update(vertices.data(), vertexCount * sizeof(glm::vec2));

    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);

    shader->unbind();

    vao->unbind();
}

}  // namespace sponge::platform::opengl::scene
