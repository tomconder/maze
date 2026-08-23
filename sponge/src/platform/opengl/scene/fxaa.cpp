#include "platform/opengl/scene/fxaa.hpp"

#include "logging/log.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"
#include "platform/opengl/renderer/gl.hpp"

#include <glm/glm.hpp>
#include <cstdint>

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;

FXAA::FXAA(const uint32_t width, const uint32_t height) :
    width(width), height(height) {
    initialize();
}

FXAA::~FXAA() {
    destroyFramebuffer();
}

void FXAA::initialize() {
    const auto shaderCreateInfo = renderer::ShaderCreateInfo{
        .name               = shaderName.data(),
        .vertexShaderPath   = "/shaders/glsl/screenquad.vert.glsl",
        .fragmentShaderPath = "/shaders/glsl/fxaa.frag.glsl",
    };
    shader = AssetManager::createShader(shaderCreateInfo);

    createFramebuffer();
}

void FXAA::createFramebuffer() {
    // RGB16F, not RGB8. This holds the tone-mapped image on its way into the
    // filter, and an 8-bit target here would quantise it a second time — once
    // undithered on the way in, once dithered on the way out — which is the
    // banding dither8 exists to prevent. Dither belongs at the final write
    // only, which is apply()'s.
    //
    // Linear filtering is required — FXAA samples between texels when blending
    // across detected edges, so nearest-neighbor would negate the entire pass.
    colorTexture = renderer::createRenderTarget(width, height, GL_RGB16F,
                                                GL_RGB, GL_FLOAT, GL_LINEAR);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           colorTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        SPONGE_GL_CRITICAL("FXAA framebuffer is not complete!");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FXAA::destroyFramebuffer() {
    if (fbo != 0) {
        glDeleteFramebuffers(1, &fbo);
        fbo = 0;
    }
    if (colorTexture != 0) {
        glDeleteTextures(1, &colorTexture);
        colorTexture = 0;
    }
}

void FXAA::begin() const {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

void FXAA::end() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FXAA::apply() const {
    // Depth testing would discard full-screen quad fragments behind anything
    // left in the depth buffer from the scene pass — disable for this blit.
    glDisable(GL_DEPTH_TEST);

    shader->bind();
    shader->setFloat2("rcpFrame", glm::vec2(1.0f / static_cast<float>(width),
                                            1.0f / static_cast<float>(height)));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorTexture);

    quad.draw();

    shader->unbind();

    glEnable(GL_DEPTH_TEST);
}

void FXAA::resize(const uint32_t newWidth, const uint32_t newHeight) {
    if (width == newWidth && height == newHeight) {
        return;
    }
    width  = newWidth;
    height = newHeight;
    destroyFramebuffer();
    createFramebuffer();
}

}  // namespace sponge::platform::opengl::scene
