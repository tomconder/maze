#include "platform/opengl/scene/screenquad.hpp"

#include "platform/opengl/renderer/gl.hpp"

#include <array>
#include <cstdint>
#include <memory>

namespace {
// Interleaved NDC position (vec2) + UV (vec2). Triangle strip rather than two
// triangles: half the vertex count and no diagonal seam, which can produce a
// one-pixel artifact on some GPUs.
constexpr std::array vertices = {
    -1.F, -1.F, 0.F, 0.F, 1.F, -1.F, 1.F, 0.F,
    -1.F, 1.F,  0.F, 1.F, 1.F, 1.F,  1.F, 1.F,
};

constexpr uint32_t vertexCount = 4;
constexpr uint32_t stride      = 4 * sizeof(float);
}  // namespace

namespace sponge::platform::opengl::scene {

ScreenQuad::ScreenQuad() {
    vao = std::make_unique<renderer::VertexArray>();
    vbo = std::make_unique<renderer::VertexBuffer>(vertices.data(),
                                                   sizeof(vertices));
    vao->setVertexBuffer(*vbo, stride);

    constexpr uint32_t positionLoc = 0;
    constexpr uint32_t texCoordLoc = 1;
    vao->addAttribute(positionLoc, 2, 0);
    vao->addAttribute(texCoordLoc, 2, 2 * sizeof(float));
}

void ScreenQuad::draw() const {
    vao->bind();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, vertexCount);
    vao->unbind();
}

}  // namespace sponge::platform::opengl::scene
