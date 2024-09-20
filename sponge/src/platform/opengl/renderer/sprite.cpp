#include "sprite.hpp"
#include "platform/opengl/renderer/resourcemanager.hpp"
#include <glm/ext/matrix_clip_space.hpp>

namespace sponge::platform::opengl::renderer {

constexpr std::string_view spriteShader = "sprite";
constexpr std::string_view vertex = "vertex";

const std::vector<uint32_t> indices = {
    0, 1, 2,  //
    0, 2, 3   //
};

Sprite::Sprite(const std::string_view name) : name(name) {
    ResourceManager::loadShader("/shaders/sprite.vert", "/shaders/sprite.frag",
                                spriteShader.data());

    const auto shader = ResourceManager::getShader(spriteShader.data());
    shader->bind();

    const auto program = shader->getId();

    vao = VertexArray::create();
    vao->bind();

    vbo = VertexBuffer::create(8);
    vbo->bind();

    ebo = IndexBuffer::create(indices);
    ebo->bind();

    if (const auto location = glGetAttribLocation(program, vertex.data());
        location != -1) {
        const auto position = static_cast<uint32_t>(location);
        glEnableVertexAttribArray(position);
        glVertexAttribPointer(position, 4, GL_FLOAT, GL_FALSE,
                              4 * sizeof(float), nullptr);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    tex = ResourceManager::getTexture(name.data());
    tex->bind();

    shader->unbind();
}

void Sprite::render(glm::vec2 position, const glm::vec2 size) const {
    const std::vector<glm::vec2> vertices = {
        { position.x + size.x, position.y },
        { 1.F, 0.F },  //
        { position.x, position.y },
        { 0.F, 0.F },  //
        { position.x, position.y + size.y },
        { 0.F, 1.F },  //
        { position.x + size.x, position.y + size.y },
        { 1.F, 1.F }
    };

    const auto shader = ResourceManager::getShader(spriteShader.data());

    vao->bind();

    shader->bind();

    tex->bind();

    vbo->update(vertices);

    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);

    shader->unbind();
}

}  // namespace sponge::platform::opengl
