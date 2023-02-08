#include "openglsprite.h"

#include <array>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "openglresourcemanager.h"

OpenGLSprite::OpenGLSprite(int screenWidth, int screenHeight) {
    std::array<float, 24> vertices = { 0.f, 1.f, 0.f, 1.f, 1.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                                       0.f, 1.f, 0.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 0.f, 1.f, 0.f };

    glm::mat4 projection = glm::ortho(0.f, screenWidth * 1.f, screenHeight * 1.f, 0.f);

    auto shader = OpenGLResourceManager::getShader("sprite");
    shader->bind();
    shader->setMat4("projection", projection);

    vao = std::make_unique<OpenGLVertexArray>();
    vao->bind();

    vbo = std::make_unique<OpenGLBuffer>(vertices.data(), (GLuint)sizeof(vertices));
    vbo->bind();

    GLuint program = OpenGLResourceManager::getShader("sprite")->getId();
    auto position = static_cast<GLuint>(glGetAttribLocation(program, "vertex"));
    glVertexAttribPointer(position, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid *)nullptr);
    glEnableVertexAttribArray(position);
}

void OpenGLSprite::render(const std::string &name, glm::vec2 position, glm::vec2 size) const {
    auto model = glm::mat4(1.f);
    model = glm::translate(model, glm::vec3(position, 0.f));

    model = glm::scale(model, glm::vec3(size, 1.f));

    std::shared_ptr<OpenGLShader> shader = OpenGLResourceManager::getShader("sprite");
    shader->bind();
    shader->setMat4("model", model);

    glActiveTexture(GL_TEXTURE0);
    OpenGLResourceManager::getTexture(name)->bind();

    vao->bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
