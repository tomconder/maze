#include "openglgrid.hpp"
#include "openglresourcemanager.hpp"

namespace sponge::renderer {

constexpr uint32_t numIndices = 6;
const std::vector<uint32_t> indices = {
    0, 2, 1,  //
    0, 3, 2   //
};

const std::vector<glm::vec2> vertices = {
    { -1.F, -1.F },  //
    { -1.F, 1.F },   //
    { 1.F, 1.F },    //
    { 1.F, -1.F },   //
};

constexpr std::string_view position = "position";

OpenGLGrid::OpenGLGrid(const std::string& shaderName) : shaderName(shaderName) {
    assert(!shaderName.empty());

    const auto shader = OpenGLResourceManager::getShader(shaderName);
    shader->bind();

    vao = std::make_unique<OpenGLVertexArray>();
    vao->bind();

    vbo = std::make_unique<OpenGLVertexBuffer>(vertices);
    vbo->bind();

    ebo = std::make_unique<OpenGLIndexBuffer>(indices);
    ebo->bind();

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

void OpenGLGrid::render() const {
    const auto shader = OpenGLResourceManager::getShader(shaderName);

    vao->bind();

    shader->bind();

    glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, nullptr);

    shader->unbind();

    glBindVertexArray(0);
}

}  // namespace sponge::renderer
