#include "renderer/opengl/openglsprite.h"

#include <array>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "openglresourcemanager.h"

namespace Sponge {

OpenGLSprite::OpenGLSprite(std::string_view name) : name(name) {
    auto shader = OpenGLResourceManager::getShader("sprite");
    shader->bind();

    vao = std::make_unique<OpenGLVertexArray>();
    vao->bind();

    vbo = std::make_unique<OpenGLBuffer>(static_cast<uint32_t>(sizeof(float)) * 16);
    vbo->bind();

    ebo = std::make_unique<OpenGLElementBuffer>(indices.data(), static_cast<uint32_t>(sizeof(uint32_t) * 6));
    ebo->bind();

    uint32_t program = shader->getId();
    if (auto location = glGetAttribLocation(program, "vertex"); location != -1) {
        auto position = static_cast<uint32_t>(location);
        glEnableVertexAttribArray(position);
        glVertexAttribPointer(position, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)nullptr);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    shader->unbind();
}

void OpenGLSprite::render(glm::vec2 position, glm::vec2 size) const {
    std::array<float, 16> vertices = { position.x,          position.y + size.y, 0.f, 0.f,  //
                                       position.x,          position.y,          0.f, 1.f,  //
                                       position.x + size.x, position.y,          1.f, 1.f,  //
                                       position.x + size.x, position.y + size.y, 1.f, 0.f };

    vao->bind();

    auto shader = OpenGLResourceManager::getShader("sprite");
    shader->bind();

    auto tex = OpenGLResourceManager::getTexture(name);
    tex->bind();

    vbo->setData(vertices.data(), static_cast<uint32_t>(vertices.size() * sizeof(float)));

    glClear(GL_DEPTH_BUFFER_BIT);

    glDrawElements(GL_TRIANGLES, (GLint)indices.size(), GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);
    shader->unbind();
}

}  // namespace Sponge
