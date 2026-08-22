#include "platform/opengl/scene/scenetarget.hpp"

#include "logging/log.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"
#include "platform/opengl/renderer/gl.hpp"

#include <array>
#include <cstdint>
#include <memory>

namespace {
constexpr std::array quadVertices = {
    -1.F, -1.F, 0.F, 0.F, 1.F, -1.F, 1.F, 0.F,
    -1.F, 1.F,  0.F, 1.F, 1.F, 1.F,  1.F, 1.F,
};
constexpr uint32_t quadVertexCount = 4;
constexpr uint32_t quadStride      = 4 * sizeof(float);
}  // namespace

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
        .name               = shaderName.data(),
        .vertexShaderPath   = "/shaders/glsl/screenquad.vert.glsl",
        .fragmentShaderPath = "/shaders/glsl/tonemap.frag.glsl",
    });

    vao = renderer::VertexArray::create();
    vao->bind();
    vbo = std::make_unique<renderer::VertexBuffer>(quadVertices.data(),
                                                   sizeof(quadVertices));
    vbo->bind();

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, quadStride,
                          reinterpret_cast<const void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, quadStride,
                          reinterpret_cast<const void*>(2 * sizeof(float)));

    vao->unbind();

    createFramebuffer();
}

void SceneTarget::createFramebuffer() {
    // RGB16F, not RGB8: the whole point of this target is that radiance above
    // 1.0 survives as far as the bloom extract and the tone map.
    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, static_cast<GLsizei>(width),
                 static_cast<GLsizei>(height), 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // GL_DEPTH_COMPONENT24 to match the prepass FBO — blitDepthToCurrentFbo()
    // requires identical depth formats.
    glGenRenderbuffers(1, &depthRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
                          static_cast<GLsizei>(width),
                          static_cast<GLsizei>(height));
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           colorTexture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, depthRbo);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        SPONGE_GL_CRITICAL("Scene framebuffer is not complete!");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SceneTarget::destroyFramebuffer() {
    if (fbo != 0) {
        glDeleteFramebuffers(1, &fbo);
        fbo = 0;
    }
    if (colorTexture != 0) {
        glDeleteTextures(1, &colorTexture);
        colorTexture = 0;
    }
    if (depthRbo != 0) {
        glDeleteRenderbuffers(1, &depthRbo);
        depthRbo = 0;
    }
}

void SceneTarget::begin() const {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

void SceneTarget::end() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SceneTarget::resolve(const uint32_t bloomTexId, const float bloomIntensity,
                          const bool ditherOutput) const {
    // Depth testing would discard the full-screen quad behind whatever the
    // scene pass left in the depth buffer.
    glDisable(GL_DEPTH_TEST);

    shader->bind();
    shader->setFloat("bloomIntensity", bloomIntensity);
    shader->setFloat("ditherOutput", ditherOutput ? 1.F : 0.F);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, bloomTexId);

    vao->bind();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, quadVertexCount);
    vao->unbind();

    glActiveTexture(GL_TEXTURE0);
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
