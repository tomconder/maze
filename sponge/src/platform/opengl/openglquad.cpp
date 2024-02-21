#include "platform/opengl/openglquad.h"
#include "core/file.h"
#include "platform/opengl/openglresourcemanager.h"

namespace sponge::renderer {

constexpr uint32_t indices[6] = {
    0, 2, 1,  //
    0, 3, 2   //
};

constexpr std::string_view quadShader = "quad";

OpenGLQuad::OpenGLQuad() {
    const auto assetsFolder = sponge::File::getResourceDir();

    OpenGLResourceManager::loadShader(assetsFolder + "/shaders/quad.vert",
                                      assetsFolder + "/shaders/quad.frag",
                                      quadShader.data());

    const auto shader = OpenGLResourceManager::getShader(quadShader.data());
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

    auto location = glGetAttribLocation(program, "position");
    if (location != -1) {
        const auto position = static_cast<uint32_t>(location);
        glEnableVertexAttribArray(position);
        glVertexAttribPointer(position, 2, GL_FLOAT, GL_FALSE,
                              2 * sizeof(GLfloat),
                              reinterpret_cast<const void*>(0));
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    shader->unbind();
}

void OpenGLQuad::render(const glm::vec2& top, const glm::vec2& bottom,
                        const glm::vec4& color) const {
    const float vertices[8] = {
        top.x,    bottom.y,  //
        top.x,    top.y,     //
        bottom.x, top.y,     //
        bottom.x, bottom.y   //
    };

    const auto shader = OpenGLResourceManager::getShader(quadShader.data());

    auto id = std::to_string(shader->getId());

    auto vao = OpenGLResourceManager::getVertexArray(id);
    vao->bind();

    shader->bind();
    shader->setFloat4("color", color);

    auto vbo = OpenGLResourceManager::getBuffer(id);
    vbo->setData(vertices, sizeof(vertices));

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);

    shader->unbind();
}

}  // namespace sponge::renderer
