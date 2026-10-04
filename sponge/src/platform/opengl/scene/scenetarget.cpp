#include "platform/opengl/scene/scenetarget.hpp"

#include "logging/log.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"
#include "platform/opengl/renderer/gl.hpp"

#include <array>
#include <cstdint>

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;

SceneTarget::SceneTarget(const uint32_t width, const uint32_t height) :
    width(width), height(height) {
    initialize();
}

SceneTarget::~SceneTarget() {
    destroyFramebuffer();
}

void SceneTarget::initialize() {
    shader = AssetManager::createShader(renderer::ShaderCreateInfo{
        .name           = shaderName.data(),
        .vertexShader   = "screenquad.vert",
        .fragmentShader = "tonemap.frag",
    });

    createFramebuffer();
}

void SceneTarget::createFramebuffer() {
    // RGB16F, not RGB8: the whole point of this target is that radiance above
    // 1.0 survives as far as the bloom extract and the tone map.
    colorTexture =
        renderer::createRenderTarget(width, height, GL_RGB16F, GL_LINEAR);
    colorCopy =
        renderer::createRenderTarget(width, height, GL_RGB16F, GL_LINEAR);

    // GL_DEPTH_COMPONENT24 to match the prepass FBO — blitDepthToCurrentFbo()
    // requires identical depth formats.
    depthRbo = renderer::createDepthRenderbuffer(width, height);
    fbo      = renderer::createFramebuffer("Scene", colorTexture, depthRbo);

    glCreateFramebuffers(1, &glassFbo);
}

void SceneTarget::destroyFramebuffer() {
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &colorTexture);
    glDeleteTextures(1, &colorCopy);
    glDeleteFramebuffers(1, &glassFbo);
    glDeleteRenderbuffers(1, &depthRbo);
    fbo = colorTexture = colorCopy = glassFbo = depthRbo = 0;
}

void SceneTarget::begin() const {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

void SceneTarget::end() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

uint32_t SceneTarget::copyColor() const {
    glCopyImageSubData(colorTexture, GL_TEXTURE_2D, 0, 0, 0, 0, colorCopy,
                       GL_TEXTURE_2D, 0, 0, 0, 0, static_cast<GLsizei>(width),
                       static_cast<GLsizei>(height), 1);
    return colorCopy;
}

bool SceneTarget::beginGlass(const uint32_t velocityTex,
                             const bool     writeVelocity) const {
    glNamedFramebufferTexture(glassFbo, GL_COLOR_ATTACHMENT0, colorTexture, 0);
    glNamedFramebufferTexture(glassFbo, GL_COLOR_ATTACHMENT1, velocityTex, 0);
    glNamedFramebufferRenderbuffer(glassFbo, GL_DEPTH_ATTACHMENT,
                                   GL_RENDERBUFFER, depthRbo);
    constexpr std::array<GLenum, 2> drawBuffers = { GL_COLOR_ATTACHMENT0,
                                                    GL_COLOR_ATTACHMENT1 };
    glNamedFramebufferDrawBuffers(glassFbo, 2, drawBuffers.data());
    glBindFramebuffer(GL_FRAMEBUFFER, glassFbo);
    glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glColorMaski(1, writeVelocity ? GL_TRUE : GL_FALSE,
                 writeVelocity ? GL_TRUE : GL_FALSE, GL_FALSE, GL_FALSE);
    if (glCheckNamedFramebufferStatus(glassFbo, GL_FRAMEBUFFER) !=
        GL_FRAMEBUFFER_COMPLETE) {
        SPONGE_GL_CRITICAL("Glass framebuffer is not complete!");
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }
    return true;
}

void SceneTarget::endGlass() const {
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SceneTarget::blitDepthTo(const uint32_t destFbo, const int w,
                              const int h) const {
    glBlitNamedFramebuffer(glassFbo, destFbo, 0, 0, w, h, 0, 0, w, h,
                           GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}

void SceneTarget::resolve(const uint32_t bloomTexId, const float bloomIntensity,
                          const bool       ditherOutput,
                          const ToneMapper toneMapper) const {
    // Depth testing would discard the full-screen quad behind whatever the
    // scene pass left in the depth buffer.
    glDisable(GL_DEPTH_TEST);

    shader->bind();
    shader->setFloat("bloomIntensity", bloomIntensity);
    shader->setBoolean("ditherOutput", ditherOutput);
    shader->setInteger("toneMapper", static_cast<int>(toneMapper));

    glBindTextureUnit(0, colorTexture);
    glBindTextureUnit(1, bloomTexId);

    quad.draw();

    shader->unbind();

    glEnable(GL_DEPTH_TEST);
}

void SceneTarget::resize(const uint32_t newWidth, const uint32_t newHeight) {
    if (width == newWidth && height == newHeight) {
        return;
    }
    width  = newWidth;
    height = newHeight;
    destroyFramebuffer();
    createFramebuffer();
}

}  // namespace sponge::platform::opengl::scene
