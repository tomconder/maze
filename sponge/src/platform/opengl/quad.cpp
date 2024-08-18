#include "quad.hpp"
#include "platform/opengl/resourcemanager.hpp"

namespace sponge::platform::opengl {

const std::vector<uint32_t> indices = {
    0, 2, 1,  //
    0, 3, 2   //
};

constexpr std::string_view position = "position";

Quad::Quad(const std::string& shaderName) : shaderName(shaderName) {
    assert(!shaderName.empty());

    const auto shader = ResourceManager::getShader(shaderName);
    shader->bind();

    vao = VertexArray::create();
    vao->bind();

    vbo = VertexBuffer::create(4);
    vbo->bind();

    ebo = IndexBuffer::create(indices);
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

void Quad::render(const glm::vec2& top, const glm::vec2& bottom,
                  const glm::vec4& color) const {
    const std::vector<glm::vec2> vertices = {
        { top.x, bottom.y },    //
        { top.x, top.y },       //
        { bottom.x, top.y },    //
        { bottom.x, bottom.y }  //
    };

    const auto shader = ResourceManager::getShader(shaderName);

    vao->bind();

    shader->bind();
    shader->setFloat4("color", color);

    vbo->update(vertices);

    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);

    shader->unbind();

    glBindVertexArray(0);
}

}  // namespace sponge::platform::opengl
