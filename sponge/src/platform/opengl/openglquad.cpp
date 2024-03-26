#include "platform/opengl/openglquad.hpp"
#include "platform/opengl/gl.hpp"
#include "platform/opengl/openglresourcemanager.hpp"

namespace sponge::renderer {

constexpr uint32_t numIndices = 6;
constexpr uint32_t indices[numIndices] = {
    0, 2, 1,  //
    0, 3, 2   //
};

constexpr std::string_view position = "position";

OpenGLQuad::OpenGLQuad(const std::string& shaderName) : shaderName(shaderName) {
    assert(!shaderName.empty());

    const auto shader = OpenGLResourceManager::getShader(shaderName);
    shader->bind();

    vao = std::make_unique<OpenGLVertexArray>();
    vao->bind();

    vbo = std::make_unique<OpenGLBuffer>(static_cast<uint32_t>(sizeof(float)) *
                                         16);
    vbo->bind();

    ebo = std::make_unique<OpenGLElementBuffer>(
        indices, static_cast<uint32_t>(sizeof(uint32_t)) * numIndices);
    ebo->bind();
    ebo->setData(indices, sizeof(indices));

    const auto program = shader->getId();

    auto location = glGetAttribLocation(program, position.data());
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

    const auto shader = OpenGLResourceManager::getShader(shaderName);

    vao->bind();

    shader->bind();
    shader->setFloat4("color", color);

    vbo->setData(vertices, sizeof(vertices));

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);

    shader->unbind();
}

}  // namespace sponge::renderer
