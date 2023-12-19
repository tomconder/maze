#include "platform/opengl/openglquad.h"
#include "platform/opengl/openglresourcemanager.h"

namespace sponge::graphics::renderer {

OpenGLQuad::OpenGLQuad() {
    const auto shader = OpenGLResourceManager::getShader("quad");
    shader->bind();

    vao = std::make_unique<OpenGLVertexArray>();
    vao->bind();

    vbo = std::make_unique<OpenGLBuffer>(static_cast<uint32_t>(sizeof(float)) *
                                         numVertices * 2);
    vbo->bind();

    constexpr uint32_t indices[numIndices] = {
        0, 2, 1,  //
        0, 3, 2   //
    };

    ebo = std::make_unique<OpenGLElementBuffer>(
        static_cast<uint32_t>(sizeof(indices)));
    ebo->bind();
    ebo->setData(indices, sizeof(indices));

    const auto program = shader->getId();
    const auto position =
        static_cast<uint32_t>(glGetAttribLocation(program, "position"));
    glEnableVertexAttribArray(position);
    glVertexAttribPointer(position, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat),
                          nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    shader->unbind();
}

void OpenGLQuad::render(const glm::vec2& top, const glm::vec2& bottom,
                        const glm::vec4& color) {
    const float vertices[numVertices] = {
        top.x,    bottom.y,  //
        top.x,    top.y,     //
        bottom.x, top.y,     //
        bottom.x, bottom.y   //
    };

    vao->bind();

    const auto shader = OpenGLResourceManager::getShader("quad");
    shader->bind();
    shader->setFloat4("color", color);

    vbo->setData(vertices, static_cast<uint32_t>(sizeof(vertices)));

    glDrawElements(GL_TRIANGLES, static_cast<int32_t>(numIndices),
                   GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);
}

}  // namespace sponge::graphics::renderer
