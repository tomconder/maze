#include "quad.hpp"
#include "platform/opengl/renderer/resourcemanager.hpp"
#include <array>

namespace {
constexpr char vertex[] = "position";
constexpr uint32_t indices[] = {
    0, 2, 1,  //
    0, 3, 2   //
};
constexpr uint32_t numVertices = 4;
}  // namespace

namespace sponge::platform::opengl::scene {

Quad::Quad() {
    shader = renderer::ResourceManager::loadShader(
        shaderName, "/shaders/quad.vert.glsl", "/shaders/quad.frag.glsl");
    shader->bind();

    vao = renderer::VertexArray::create();
    vao->bind();

    vbo = std::make_unique<renderer::VertexBuffer>(
        nullptr, numVertices * sizeof(glm::vec2));
    vbo->bind();

    ebo = renderer::IndexBuffer::create({ indices, std::end(indices) });
    ebo->bind();

    const auto program = shader->getId();

    if (const auto location = glGetAttribLocation(program, vertex);
        location != -1) {
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
    const std::array<glm::vec2, numVertices> vertices{ {
        { top.x, bottom.y },    //
        { top.x, top.y },       //
        { bottom.x, top.y },    //
        { bottom.x, bottom.y }  //
    } };

    vao->bind();

    shader->bind();
    shader->setFloat4("color", color);

    vbo->update(vertices.data(), numVertices * sizeof(glm::vec2));

    glDrawElements(GL_TRIANGLES, std::size(indices), GL_UNSIGNED_INT, nullptr);

    shader->unbind();

    glBindVertexArray(0);
}

}  // namespace sponge::platform::opengl::scene
