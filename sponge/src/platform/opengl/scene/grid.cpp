#include "grid.hpp"
#include "platform/opengl/renderer/resourcemanager.hpp"
#include <array>

namespace {
constexpr char position[] = "position";
constexpr uint32_t indices[] = {
    0, 2, 1,  //
    0, 3, 2   //
};
constexpr glm::vec2 vertices[] = {
    { -1.F, -1.F },  //
    { -1.F, 1.F },   //
    { 1.F, 1.F },    //
    { 1.F, -1.F },   //
};
}  // namespace

namespace sponge::platform::opengl::scene {

Grid::Grid() {
    shader = renderer::ResourceManager::loadShader(
        shaderName, "/shaders/infinitegrid.vert", "/shaders/infinitegrid.frag");
    shader->bind();

    vao = renderer::VertexArray::create();
    vao->bind();

    vbo = std::make_unique<renderer::VertexBuffer>(vertices,
                                                       sizeof(vertices));
    vbo->bind();

    ebo = renderer::IndexBuffer::create({ indices, std::end(indices) });
    ebo->bind();

    const auto program = shader->getId();

    auto location = glGetAttribLocation(program, position);
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

void Grid::render() const {
    vao->bind();

    shader->bind();

    glDrawElements(GL_TRIANGLES, std::size(indices), GL_UNSIGNED_INT, nullptr);

    shader->unbind();

    glBindVertexArray(0);
}

}  // namespace sponge::platform::opengl::scene
