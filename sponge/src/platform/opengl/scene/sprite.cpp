#include "platform/opengl/scene/sprite.hpp"

#include "platform/opengl/renderer/assetmanager.hpp"

#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <memory>
#include <string>

namespace {
constexpr std::array<uint32_t, 6> indices     = { 0, 1, 2, 0, 2, 3 };
constexpr uint32_t                indexCount  = 6;
constexpr uint32_t                vertexCount = 8;
}  // namespace

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;

inline const std::string Sprite::shaderName = "sprite";

Sprite::Sprite(const std::string& name, const std::string& texturePath) {
    createBuffers();

    const renderer::TextureCreateInfo textureCreateInfo{
        .name = name,
        .path = texturePath,
    };
    tex = AssetManager::createTexture(textureCreateInfo);
}

Sprite::Sprite(std::shared_ptr<renderer::Texture> texture,
               const glm::vec2& uvOffset, const glm::vec2& uvScale) :
    tex(std::move(texture)), uvOffset(uvOffset), uvScale(uvScale) {
    createBuffers();
}

void Sprite::createBuffers() {
    const auto shaderCreateInfo = renderer::ShaderCreateInfo{
        .name           = shaderName,
        .vertexShader   = "sprite.vert",
        .fragmentShader = "sprite.frag",
    };
    shader = AssetManager::createShader(shaderCreateInfo);

    vao = std::make_unique<renderer::VertexArray>();
    vbo = std::make_unique<renderer::VertexBuffer>(
        nullptr, vertexCount * sizeof(glm::vec2));
    ebo = std::make_unique<renderer::IndexBuffer>(indices.data(),
                                                  sizeof(indices));
    vao->setVertexBuffer(*vbo, 4 * sizeof(float));
    vao->setIndexBuffer(*ebo);

    constexpr uint32_t position = 0;
    vao->addAttribute(position, 4, 0);
}

void Sprite::render(const glm::vec2& position, const glm::vec2& size,
                    const std::optional<float> alpha) const {
    // An atlas sprite maps the unit quad onto its own rect; a whole-texture
    // sprite leaves uvOffset/uvScale at 0/1 and gets the full range.
    const auto uv = [this](const float u, const float v) {
        return glm::vec2{ uvOffset.x + (u * uvScale.x),
                          uvOffset.y + (v * uvScale.y) };
    };

    const std::array<glm::vec2, vertexCount> vertices{
        { { position.x + size.x, position.y },
          uv(1.F, 0.F),
          { position.x, position.y },
          uv(0.F, 0.F),
          { position.x, position.y + size.y },
          uv(0.F, 1.F),
          { position.x + size.x, position.y + size.y },
          uv(1.F, 1.F) },
    };

    vao->bind();

    shader->bind();
    shader->setFloat("alpha", alpha.value_or(1.0F));

    tex->bind(0);

    vbo->update(vertices.data(), vertexCount * sizeof(glm::vec2));

    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);

    shader->unbind();

    vao->unbind();
}

}  // namespace sponge::platform::opengl::scene
