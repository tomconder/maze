#include "lightcube.hpp"
#include "logging/log.hpp"
#include "platform/opengl/renderer/resourcemanager.hpp"

namespace {
constexpr char position[] = "position";
constexpr glm::vec3 vertices[] = {
    { -0.5, 0.5, -0.5 }, { -0.5, 0.5, 0.5 },   { 0.5, 0.5, 0.5 },
    { -0.5, 0.5, -0.5 }, { 0.5, 0.5, 0.5 },    { 0.5, 0.5, -0.5 },
    { -0.5, 0.5, -0.5 }, { -0.5, -0.5, -0.5 }, { -0.5, -0.5, 0.5 },
    { -0.5, 0.5, -0.5 }, { -0.5, -0.5, 0.5 },  { -0.5, 0.5, 0.5 },
    { 0.5, 0.5, 0.5 },   { 0.5, -0.5, 0.5 },   { 0.5, -0.5, -0.5 },
    { 0.5, 0.5, 0.5 },   { 0.5, -0.5, -0.5 },  { 0.5, 0.5, -0.5 },
    { 0.5, 0.5, -0.5 },  { 0.5, -0.5, -0.5 },  { -0.5, -0.5, -0.5 },
    { 0.5, 0.5, -0.5 },  { -0.5, -0.5, -0.5 }, { -0.5, 0.5, -0.5 },
    { -0.5, 0.5, 0.5 },  { -0.5, -0.5, 0.5 },  { 0.5, -0.5, 0.5 },
    { -0.5, 0.5, 0.5 },  { 0.5, -0.5, 0.5 },   { 0.5, 0.5, 0.5 },
    { -0.5, -0.5, 0.5 }, { -0.5, -0.5, -0.5 }, { 0.5, -0.5, -0.5 },
    { -0.5, -0.5, 0.5 }, { 0.5, -0.5, -0.5 },  { 0.5, -0.5, 0.5 }
};
}  // namespace

namespace sponge::platform::opengl::scene {

LightCube::LightCube() {
    shader = renderer::ResourceManager::loadShader(
        shaderName, "/shaders/lightcube.vert.glsl",
        "/shaders/lightcube.frag.glsl");
    shader->bind();

    vao = renderer::VertexArray::create();
    vao->bind();

    vbo = std::make_unique<renderer::VertexBuffer>(vertices, sizeof(vertices));
    vbo->bind();

    const auto program = shader->getId();
    if (const auto location = glGetAttribLocation(program, position);
        location != -1) {
        const auto pos = static_cast<uint32_t>(location);
        glEnableVertexAttribArray(pos);
        glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3),
                              reinterpret_cast<const void*>(0));
    }

    shader->unbind();
    vao->unbind();
}

void LightCube::render() const {
    vao->bind();

    shader->bind();

    glDrawArrays(GL_TRIANGLES, 0, std::size(vertices));

    shader->unbind();

    vao->unbind();
}

}  // namespace sponge::platform::opengl::scene
