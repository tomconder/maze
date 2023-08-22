#include "renderer/opengl/openglsprite.h"
#include "openglresourcemanager.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <array>

namespace sponge {

OpenGLSprite::OpenGLSprite(std::string_view name) : name(name) {
    auto shader = OpenGLResourceManager::getShader("sprite");
    shader->bind();

    vao = std::make_unique<OpenGLVertexArray>();
    vao->bind();

    vbo = std::make_unique<OpenGLBuffer>(static_cast<uint32_t>(sizeof(float)) *
                                         16);
    vbo->bind();

    constexpr uint32_t indices[numIndices] = {
        0, 1, 2,  //
        0, 2, 3   //
    };

    ebo = std::make_unique<OpenGLElementBuffer>(
        indices, static_cast<uint32_t>(sizeof(indices)));
    ebo->bind();
    ebo->setData(indices, static_cast<uint32_t>(sizeof(indices)));

    uint32_t program = shader->getId();
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
        position.x,          position.y + size.y, 0.F, 1.F,  //
        position.x,          position.y,          0.F, 0.F,  //
        position.x + size.x, position.y,          1.F, 0.F,  //
        position.x + size.x, position.y + size.y, 1.F, 1.F
    };

    vao->bind();

    const auto shader = OpenGLResourceManager::getShader("sprite");
    shader->bind();

    const auto tex = OpenGLResourceManager::getTexture(name);
    tex->bind();

    vbo->setData(vertices, static_cast<uint32_t>(sizeof(vertices)));

    glClear(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_CULL_FACE);

    glDrawElements(GL_TRIANGLES, static_cast<GLint>(numIndices),
                   GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);
    glEnable(GL_CULL_FACE);

    shader->unbind();
}

}  // namespace sponge
