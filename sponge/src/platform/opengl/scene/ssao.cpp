#include "platform/opengl/scene/ssao.hpp"

#include "logging/log.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"
#include "platform/opengl/renderer/gl.hpp"

#include <array>
#include <cstdint>
#include <random>

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;

Ssao::Ssao(const uint32_t width, const uint32_t height) :
    width(width), height(height) {
    initialize();
}

Ssao::~Ssao() {
    destroyFramebuffers();
    glDeleteTextures(1, &noiseTexture);
}

void Ssao::initialize() {
    ssaoShader = AssetManager::createShader(renderer::ShaderCreateInfo{
        .name           = "ssao",
        .vertexShader   = "screenquad.vert",
        .fragmentShader = "ssao.frag",
    });
    blurShader = AssetManager::createShader(renderer::ShaderCreateInfo{
        .name           = "ssao_blur",
        .vertexShader   = "screenquad.vert",
        .fragmentShader = "ssao_blur.frag",
    });

    createNoiseTexture();
    createFramebuffers();
}

void Ssao::createNoiseTexture() {
    // Random rotation vectors around the tangent-space Z axis (z left at 0,
    // as in the reference technique). Fixed seed: deterministic across runs,
    // no need for the noise pattern itself to vary.
    // NOLINTNEXTLINE(bugprone-random-generator-seed) fixed layout
    std::mt19937                   rng(1337U);
    std::uniform_real_distribution dist(-1.F, 1.F);

    std::array<float, 4 * 4 * 2> noise{};
    for (auto& v : noise) {
        v = dist(rng);
    }

    glGenTextures(1, &noiseTexture);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 4, 4, 0, GL_RG, GL_FLOAT,
                 noise.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Ssao::createFramebuffers() {
    auto makeAoFbo = [this](uint32_t& fbo, uint32_t& tex) {
        tex = renderer::createRenderTarget(width, height, GL_R16F, GL_RED,
                                           GL_FLOAT, GL_LINEAR);
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, tex, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
            GL_FRAMEBUFFER_COMPLETE) {
            SPONGE_GL_CRITICAL("SSAO framebuffer is not complete!");
        }
    };

    makeAoFbo(rawFbo, rawTexture);
    makeAoFbo(blurFbo, blurTexture);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Ssao::destroyFramebuffers() {
    glDeleteFramebuffers(1, &rawFbo);
    glDeleteTextures(1, &rawTexture);
    glDeleteFramebuffers(1, &blurFbo);
    glDeleteTextures(1, &blurTexture);
    rawFbo = rawTexture = blurFbo = blurTexture = 0;
}

void Ssao::process(const uint32_t depthTexId, const uint32_t normalTexId,
                   const glm::mat4& projection, const glm::mat4& invProjection,
                   const float radius) const {
    glDisable(GL_DEPTH_TEST);
    // The renderer leaves blending on globally, and these targets are
    // single-channel (GL_R16F, no alpha) with undefined shader alpha output:
    // left enabled, every write blended toward the framebuffer's zeroed
    // initial contents instead of landing.
    glDisable(GL_BLEND);
    glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));

    glBindFramebuffer(GL_FRAMEBUFFER, rawFbo);
    ssaoShader->bind();
    ssaoShader->setMat4("projection", projection);
    ssaoShader->setMat4("invProjection", invProjection);
    ssaoShader->setFloat2(
        "noiseScale",
        glm::vec2(static_cast<float>(width), static_cast<float>(height)) / 4.F);
    ssaoShader->setFloat("radius", radius);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthTexId);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalTexId);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    quad.draw();
    ssaoShader->unbind();

    glBindFramebuffer(GL_FRAMEBUFFER, blurFbo);
    blurShader->bind();
    blurShader->setFloat2(
        "texelSize",
        1.F / glm::vec2(static_cast<float>(width), static_cast<float>(height)));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, rawTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthTexId);
    quad.draw();
    blurShader->unbind();

    glActiveTexture(GL_TEXTURE0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
}

void Ssao::resize(const uint32_t newWidth, const uint32_t newHeight) {
    if (width == newWidth && height == newHeight) {
        return;
    }
    width  = newWidth;
    height = newHeight;
    destroyFramebuffers();
    createFramebuffers();
}

}  // namespace sponge::platform::opengl::scene
