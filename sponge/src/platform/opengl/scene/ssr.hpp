#pragma once

#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/scene/screenquad.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>

namespace sponge::platform::opengl::scene {

// Screen-space reflection: marches the reflected view ray against the depth
// prepass and blends the scene color it hits over the lit surface. The
// prepass normal alpha holds each pixel's strength, and 0 skips the pixel.
// See ssr.slang for the technique.
class Ssr {
public:
    Ssr();

    Ssr(const Ssr&)            = delete;
    Ssr& operator=(const Ssr&) = delete;

    // Draws into the bound framebuffer with alpha blending. colorTexId must
    // not be an attachment of that framebuffer. projection is the unjittered
    // camera projection, not the jittered one the prepass depth was
    // rasterized with under TAA; the sub-pixel jitter is ignored.
    void apply(uint32_t colorTexId, uint32_t depthTexId, uint32_t normalTexId,
               const glm::mat4& projection,
               const glm::mat4& invProjection) const;

private:
    std::shared_ptr<renderer::Shader> shader;
    ScreenQuad                        quad;
};

}  // namespace sponge::platform::opengl::scene
