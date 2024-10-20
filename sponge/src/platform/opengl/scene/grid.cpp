#include "grid.hpp"
#include "platform/opengl/renderer/resourcemanager.hpp"

namespace {
constexpr char position[] = "position";
constexpr uint32_t indices[] = {
    0, 2, 1,  //
    0, 3, 2   //
};
constexpr uint32_t indexCount = 6;
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
        shaderName, "/shaders/infinitegrid.vert.glsl",
        "/shaders/infinitegrid.frag.glsl");
    shader->bind();

    vao = renderer::VertexArray::create();
    vao->bind();

    vbo = std::make_unique<renderer::VertexBuffer>(vertices, sizeof(vertices));
    vbo->bind();

    ebo = std::make_unique<renderer::IndexBuffer>(indices, sizeof(indices));
    ebo->bind();

    const auto program = shader->getId();

    if (const auto location = glGetAttribLocation(program, position);
        location != -1) {
        const auto position = static_cast<uint32_t>(location);
        glEnableVertexAttribArray(position);
        glVertexAttribPointer(position, 2, GL_FLOAT, GL_FALSE,
                              2 * sizeof(GLfloat),
                              reinterpret_cast<const void*>(0));
    }

    vbo->unbind();
    vao->unbind();

    shader->unbind();
}

void Grid::render() const {
    vao->bind();

    shader->bind();

    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);

    shader->unbind();

    vao->unbind();
}

}  // namespace sponge::platform::opengl::scene
