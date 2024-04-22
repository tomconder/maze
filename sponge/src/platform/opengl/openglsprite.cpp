#include "platform/opengl/openglsprite.hpp"
#include "platform/opengl/gl.hpp"
#include "platform/opengl/openglresourcemanager.hpp"
#include <glm/ext/matrix_clip_space.hpp>

namespace sponge::renderer {

constexpr std::string_view spriteShader = "sprite";
constexpr std::string_view vertex = "vertex";

const std::vector<uint32_t> indices = {
    0, 1, 2,  //
    0, 2, 3   //
};

OpenGLSprite::OpenGLSprite(std::string_view name) : name(name) {
    OpenGLResourceManager::loadShader(
        "/shaders/sprite.vert", "/shaders/sprite.frag", spriteShader.data());

    auto shader = OpenGLResourceManager::getShader(spriteShader.data());
    shader->bind();

    const auto program = shader->getId();

    vao = std::make_unique<OpenGLVertexArray>();
    vao->bind();

    vbo = std::make_unique<OpenGLVertexBuffer>(8);
    vbo->bind();

    ebo = std::make_unique<OpenGLIndexBuffer>(indices);
    ebo->bind();

    if (auto location = glGetAttribLocation(program, vertex.data());
        location != -1) {
        auto position = static_cast<uint32_t>(location);
        glEnableVertexAttribArray(position);
        glVertexAttribPointer(position, 4, GL_FLOAT, GL_FALSE,
                              4 * sizeof(float), nullptr);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    shader->unbind();
}

void OpenGLSprite::render(glm::vec2 position, glm::vec2 size) const {
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

    const auto shader = OpenGLResourceManager::getShader(spriteShader.data());

    vao->bind();

    shader->bind();

    const auto tex = OpenGLResourceManager::getTexture(name);
    tex->bind();

    vbo->update(vertices);

    glDrawElements(GL_TRIANGLES, sizeof(indices), GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);

    shader->unbind();
}

}  // namespace sponge::renderer
