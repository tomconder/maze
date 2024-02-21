#include "platform/opengl/openglsprite.h"
#include "core/file.h"
#include "platform/opengl/openglresourcemanager.h"
#include <glm/ext/matrix_clip_space.hpp>

namespace sponge::renderer {

constexpr std::string_view spriteShader = "sprite";

static constexpr uint32_t numIndices = 6;
static constexpr uint32_t numVertices = 16;

constexpr uint32_t indices[numIndices] = {
    0, 1, 2,  //
    0, 2, 3   //
};

OpenGLSprite::OpenGLSprite(std::string_view name) : name(name) {
    const auto assetsFolder = sponge::File::getResourceDir();

    OpenGLResourceManager::loadShader(assetsFolder + "/shaders/sprite.vert",
                                      assetsFolder + "/shaders/sprite.frag",
                                      spriteShader.data());

    auto shader = OpenGLResourceManager::getShader(spriteShader.data());
    shader->bind();

    const auto program = shader->getId();
    const auto id = std::to_string(program);

    const auto vao = OpenGLResourceManager::createVertexArray(id);
    vao->bind();

    const auto vbo = OpenGLResourceManager::createBuffer(
        id, static_cast<uint32_t>(sizeof(float)) * 16);
    vbo->bind();

    const auto ebo =
        OpenGLResourceManager::createElementBuffer(id, sizeof(indices));
    ebo->bind();
    ebo->setData(indices, sizeof(indices));

    if (auto location = glGetAttribLocation(program, "vertex");
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
    const float vertices[numVertices] = {
        position.x + size.x, position.y,          1.F, 0.F,  //
        position.x,          position.y,          0.F, 0.F,  //
        position.x,          position.y + size.y, 0.F, 1.F,  //
        position.x + size.x, position.y + size.y, 1.F, 1.F
    };

    const auto shader = OpenGLResourceManager::getShader(spriteShader.data());

    auto id = std::to_string(shader->getId());

    auto vao = OpenGLResourceManager::getVertexArray(id);
    vao->bind();

    shader->bind();

    const auto tex = OpenGLResourceManager::getTexture(name);
    tex->bind();

    auto vbo = OpenGLResourceManager::getBuffer(id);
    vbo->setData(vertices, sizeof(vertices));

    glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);

    shader->unbind();
}

}  // namespace sponge::renderer
