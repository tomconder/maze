#pragma once

#include <cstdint>

namespace sponge::platform::opengl::scene {

// The depth prepass target: depth, screen-space motion and view-space normal
// in one FBO. The prepass writes all three; SSAO, SSR, TAA and the glass pass
// read them.
class DepthPrepass {
public:
    DepthPrepass() = delete;
    DepthPrepass(uint32_t width, uint32_t height);
    ~DepthPrepass();

    DepthPrepass(const DepthPrepass&)            = delete;
    DepthPrepass& operator=(const DepthPrepass&) = delete;

    // Binds the FBO and clears it for the prepass draw: depth writes on
    // (GL_LESS), blending off, velocity masked to writeVelocity, velocity and
    // normal cleared to zero. end() restores blending and the colour mask and
    // binds the default framebuffer.
    void begin(bool writeVelocity) const;
    void end() const;

    // Binds the FBO without touching any other state, for depth-test-only
    // passes (occlusion queries) against the depth just rasterized.
    void bind() const;
    void unbind() const;

    // Copies the prepass depth into the bound draw framebuffer. The target
    // must use GL_DEPTH_COMPONENT24 and be the same size.
    void blitDepthToBound() const;

    void resize(uint32_t newWidth, uint32_t newHeight);

    uint32_t getFramebuffer() const {
        return fbo;
    }

    uint32_t getDepthTexture() const {
        return depthTexture;
    }

    // Screen-space motion (RG16F, current UV minus previous UV), consumed by
    // TAA.
    uint32_t getVelocityTexture() const {
        return velocityTexture;
    }

    // View-space normal in xyz and reflection strength in w (RGBA16F). SSAO
    // reads xyz and SSR reads w.
    uint32_t getNormalTexture() const {
        return normalTexture;
    }

private:
    uint32_t fbo             = 0;
    uint32_t depthTexture    = 0;
    uint32_t velocityTexture = 0;
    uint32_t normalTexture   = 0;

    uint32_t width  = 0;
    uint32_t height = 0;

    void createFramebuffer();
    void destroyFramebuffer();
};

}  // namespace sponge::platform::opengl::scene
