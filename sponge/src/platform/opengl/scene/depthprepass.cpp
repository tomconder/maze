#include "platform/opengl/scene/depthprepass.hpp"

#include "platform/opengl/renderer/gl.hpp"

#include <array>

namespace sponge::platform::opengl::scene {

DepthPrepass::DepthPrepass(const uint32_t width, const uint32_t height) :
    width(width), height(height) {
    createFramebuffer();
}

DepthPrepass::~DepthPrepass() {
    destroyFramebuffer();
}

void DepthPrepass::createFramebuffer() {
    depthTexture = renderer::createRenderTarget(
        width, height, GL_DEPTH_COMPONENT24, GL_NEAREST);

    // RG16F holds a UV delta, not an absolute UV: at 1920 wide a one-pixel
    // motion is ~5e-4, which a half float carries accurately as a delta and
    // would quantise to worse than a pixel as an absolute coordinate.
    velocityTexture =
        renderer::createRenderTarget(width, height, GL_RG16F, GL_NEAREST);

    // RGBA16F: RGB carries signed unit components directly, no [0,1]
    // encode/decode needed; alpha carries the SSR reflection strength.
    normalTexture =
        renderer::createRenderTarget(width, height, GL_RGBA16F, GL_NEAREST);

    glCreateFramebuffers(1, &fbo);
    glNamedFramebufferTexture(fbo, GL_DEPTH_ATTACHMENT, depthTexture, 0);
    glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, velocityTexture, 0);
    glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT1, normalTexture, 0);
    constexpr std::array<GLenum, 2> drawBuffers = { GL_COLOR_ATTACHMENT0,
                                                    GL_COLOR_ATTACHMENT1 };
    glNamedFramebufferDrawBuffers(fbo, 2, drawBuffers.data());
    glNamedFramebufferReadBuffer(fbo, GL_NONE);
    if (glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) !=
        GL_FRAMEBUFFER_COMPLETE) {
        SPONGE_GL_CRITICAL("Depth prepass framebuffer is not complete!");
    }
}

void DepthPrepass::destroyFramebuffer() {
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &depthTexture);
    glDeleteTextures(1, &velocityTexture);
    glDeleteTextures(1, &normalTexture);
    fbo = depthTexture = velocityTexture = normalTexture = 0;
}

void DepthPrepass::begin(const bool writeVelocity) const {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    // Indexed mask: attachment 0 (velocity) follows writeVelocity, attachment
    // 1 (normal) is always live — SSAO and SSR both read it every frame
    // regardless of AA mode.
    glColorMaski(0, writeVelocity ? GL_TRUE : GL_FALSE,
                 writeVelocity ? GL_TRUE : GL_FALSE, GL_FALSE, GL_FALSE);
    glColorMaski(1, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Blending is enabled globally (RendererAPI) and never turned off, so the
    // restore in end() is unconditional. Without this, the shader's writes
    // blend against the clear colour instead of landing untouched — wrong for
    // motion vectors and for normals alike.
    glDisable(GL_BLEND);
    if (writeVelocity) {
        // Explicit zero rather than glClear, which would use the global grey
        // clear colour and read back as ~22 pixels of bogus motion.
        constexpr std::array noMotion = { 0.F, 0.F, 0.F, 0.F };
        glClearNamedFramebufferfv(fbo, GL_COLOR, 0, noMotion.data());
    }
    // Zero normal reads as "no surface here" (see ssao.slang), which is
    // exactly right for pixels the prepass never draws to.
    constexpr std::array noNormal = { 0.F, 0.F, 0.F, 0.F };
    glClearNamedFramebufferfv(fbo, GL_COLOR, 1, noNormal.data());
}

void DepthPrepass::end() const {
    glEnable(GL_BLEND);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DepthPrepass::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

void DepthPrepass::unbind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DepthPrepass::blitDepthToBound() const {
    GLint drawFbo = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFbo);
    const auto w = static_cast<GLint>(width);
    const auto h = static_cast<GLint>(height);
    glBlitNamedFramebuffer(fbo, static_cast<GLuint>(drawFbo), 0, 0, w, h, 0, 0,
                           w, h, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}

void DepthPrepass::resize(const uint32_t newWidth, const uint32_t newHeight) {
    if (width == newWidth && height == newHeight) {
        return;
    }
    width  = newWidth;
    height = newHeight;
    destroyFramebuffer();
    createFramebuffer();
}

}  // namespace sponge::platform::opengl::scene
