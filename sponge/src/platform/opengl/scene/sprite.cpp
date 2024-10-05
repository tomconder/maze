#include "sprite.hpp"
#include "platform/opengl/renderer/resourcemanager.hpp"
#include <glm/ext/matrix_clip_space.hpp>
#include <array>

namespace {
constexpr char vertex[] = "vertex";
constexpr uint32_t indices[] = {
    0, 1, 2,  //
    0, 2, 3   //
};
constexpr uint32_t numVertices = 8;
}  // namespace

namespace sponge::platform::opengl::scene {

Sprite::Sprite(const std::string& name, const std::string& texturePath) {
    shader = renderer::ResourceManager::loadShader(
        shaderName, "/shaders/sprite.vert.glsl", "/shaders/sprite.frag.glsl");
    shader->bind();

    vao = renderer::VertexArray::create();
    vao->bind();

    vbo = std::make_unique<renderer::VertexBuffer>(
        nullptr, numVertices * sizeof(glm::vec2));
    vbo->bind();

    ebo = renderer::IndexBuffer::create({ indices, std::end(indices) });
    ebo->bind();

    const auto program = shader->getId();

    if (const auto location = glGetAttribLocation(program, vertex);
        location != -1) {
        const auto position = static_cast<uint32_t>(location);
        glEnableVertexAttribArray(position);
        glVertexAttribPointer(position, 4, GL_FLOAT, GL_FALSE,
                              4 * sizeof(float), nullptr);
    }

    vbo->unbind();
    vao->unbind();

    tex = renderer::ResourceManager::loadTexture(name, texturePath);
    tex->bind();

    shader->unbind();
}

void Sprite::render(const glm::vec2& position, const glm::vec2& size) const {
    const std::array<glm::vec2, numVertices> vertices{
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

    vbo->update(vertices.data(), numVertices * sizeof(glm::vec2));

    glDrawElements(GL_TRIANGLES, std::size(indices), GL_UNSIGNED_INT, nullptr);

    shader->unbind();

    vao->unbind();
}

}  // namespace sponge::platform::opengl::scene
