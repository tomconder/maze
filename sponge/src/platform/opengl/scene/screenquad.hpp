#pragma once

#include "platform/opengl/renderer/vertexarray.hpp"
#include "platform/opengl/renderer/vertexbuffer.hpp"

#include <memory>

namespace sponge::platform::opengl::scene {

// Full-screen triangle strip shared by every post-processing pass. All of them
// use screenquad.vert.glsl, so the vertex layout is fixed: location 0 is the
// NDC position, location 1 the UV.
class ScreenQuad {
public:
    ScreenQuad();

    void draw() const;

private:
    std::unique_ptr<renderer::VertexArray>  vao;
    std::unique_ptr<renderer::VertexBuffer> vbo;
};

}  // namespace sponge::platform::opengl::scene
