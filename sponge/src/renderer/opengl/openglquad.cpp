#include "renderer/opengl/openglquad.h"
#include "renderer/opengl/openglresourcemanager.h"

namespace sponge {

OpenGLQuad::OpenGLQuad() {
    auto shader = OpenGLResourceManager::getShader("quad");
    shader->bind();

    vao = std::make_unique<OpenGLVertexArray>();
    vao->bind();

    vbo = std::make_unique<OpenGLBuffer>(static_cast<uint32_t>(sizeof(float)) *
                                         16);
    vbo->bind();

    ebo = std::make_unique<OpenGLElementBuffer>(
        static_cast<uint32_t>(sizeof(uint32_t)) * 6);
    ebo->bind();

    uint32_t program = shader->getId();
    auto position =
        static_cast<uint32_t>(glGetAttribLocation(program, "position"));
    glEnableVertexAttribArray(position);
    glVertexAttribPointer(position, 4, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat),
                          nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    shader->unbind();
}

void OpenGLQuad::render(const glm::vec2& top, const glm::vec2& bottom,
                        const glm::vec4& color) {
    auto xpos = top.x;
    auto ypos = top.y;
    auto h = std::abs(bottom.y - top.y);
    auto w = std::abs(bottom.x - top.x);

    auto vertices = std::vector<float>{
        xpos,     ypos + h,  //
        xpos,     ypos,      //
        xpos + w, ypos,      //
        xpos + w, ypos + h   //
    };

    auto indices = std::vector<uint32_t>{
        0, 1, 2,  //
        0, 2, 3   //
    };

    vao->bind();

    auto shader = OpenGLResourceManager::getShader("quad");
    shader->bind();
    shader->setFloat4("color", color);

    vbo->setData(vertices.data(),
                 static_cast<uint32_t>(vertices.size() * sizeof(float)));

    ebo->setData(indices.data(),
                 static_cast<uint32_t>(indices.size() * sizeof(uint32_t)));

    glClear(GL_DEPTH_BUFFER_BIT);

    glDrawElements(GL_TRIANGLES, static_cast<GLint>(indices.size()),
                   GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);
}

}  // namespace sponge
