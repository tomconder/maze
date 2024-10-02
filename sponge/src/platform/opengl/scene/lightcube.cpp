#include "lightcube.hpp"
#include "platform/opengl/renderer/resourcemanager.hpp"

namespace sponge::platform::opengl::scene {

LightCube::LightCube() {
    shader = renderer::ResourceManager::loadShader(
        shaderName, "/shaders/lightcube.vert", "/shaders/lightcube.frag");
    shader->bind();

    vao = renderer::VertexArray::create();
    vao->bind();

    // TODO: create vbo
    // vbo = renderer::VertexBuffer::create(
    //     std::vector<glm::vec2>{ vertices, std::end(vertices) });
    // vbo->bind();

    // TODO: create ebo
    // ebo = renderer::IndexBuffer::create({ indices, std::end(indices) });
    // ebo->bind();

    shader->unbind();
}

void LightCube::render() const {
    vao->bind();

    shader->bind();

    //    glDrawElements(GL_TRIANGLES, std::size(indices), GL_UNSIGNED_INT,
    //    nullptr);

    shader->unbind();

    glBindVertexArray(0);
}

}  // namespace sponge::platform::opengl::scene
