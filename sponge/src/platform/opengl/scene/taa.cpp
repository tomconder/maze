#include "platform/opengl/scene/taa.hpp"

#include "logging/log.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"
#include "platform/opengl/renderer/gl.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <string_view>

namespace {
// Radical inverse in the given base — the Halton building block.
constexpr float halton(uint32_t index, const uint32_t base) {
    float result   = 0.F;
    float fraction = 1.F / static_cast<float>(base);
    while (index > 0) {
        result += static_cast<float>(index % base) * fraction;
        index /= base;
        fraction /= static_cast<float>(base);
    }
    return result;
}

// Base 2 is exact in binary, so these hold without an epsilon.
static_assert(halton(1, 2) == 0.5F);
static_assert(halton(2, 2) == 0.25F);
static_assert(halton(3, 2) == 0.75F);
static_assert(halton(4, 2) == 0.125F);

// Length of the jitter cycle. 8 samples resolve a 2x2 sub-pixel grid well
// past the point the 0.1 blend factor can distinguish.
constexpr uint32_t jitterSampleCount = 8;
}  // namespace

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;

TAA::TAA(const uint32_t width, const uint32_t height) :
    width(width), height(height) {
    initialize();
}

TAA::~TAA() {
    destroyFramebuffers();
}

glm::vec2 TAA::haltonJitter(const uint32_t index, const uint32_t width,
                            const uint32_t height) {
    if (width == 0 || height == 0) {
        return glm::vec2(0.F);
    }
    // Halton starts at index 1; index 0 is the degenerate (0,0) sample.
    const uint32_t i = (index % jitterSampleCount) + 1;
    // [0,1) sample → [-1,1) sub-pixel offset → NDC (2 units across the screen)
    return glm::vec2((halton(i, 2) - 0.5F) * 2.F / static_cast<float>(width),
                     (halton(i, 3) - 0.5F) * 2.F / static_cast<float>(height));
}

void TAA::initialize() {
    auto makeShader = [](std::string_view name, const char* frag) {
        return AssetManager::createShader(renderer::ShaderCreateInfo{
            .name           = std::string(name),
            .vertexShader   = "screenquad.vert",
            .fragmentShader = frag,
        });
    };

    resolveShader = makeShader(resolveShaderName, "taa_resolve.frag");
    presentShader = makeShader(presentShaderName, "taa_present.frag");

    createFramebuffers();
}

void TAA::createFramebuffers() {
    // Receives the tone-mapped image from SceneTarget::resolve(). RGB16F
    // rather than 8-bit so the history accumulates without requantising the
    // input every frame.
    sceneColorTexture =
        renderer::createRenderTarget(width, height, GL_RGB16F, GL_LINEAR);

    sceneFbo = renderer::createFramebuffer("TAA scene", sceneColorTexture);

    // Float history: with a 0.1 blend factor an 8-bit target quantises every
    // increment below 1/255 to zero and the image never converges.
    for (uint32_t i = 0; i < historyTextures.size(); i++) {
        historyTextures[i] =
            renderer::createRenderTarget(width, height, GL_RGB16F, GL_LINEAR);
        historyFbos[i] =
            renderer::createFramebuffer("TAA history", historyTextures[i]);
    }

    historyValid = false;
}

void TAA::destroyFramebuffers() {
    if (sceneFbo != 0) {
        glDeleteFramebuffers(1, &sceneFbo);
        sceneFbo = 0;
    }
    if (sceneColorTexture != 0) {
        glDeleteTextures(1, &sceneColorTexture);
        sceneColorTexture = 0;
    }
    for (uint32_t i = 0; i < historyTextures.size(); i++) {
        if (historyFbos[i] != 0) {
            glDeleteFramebuffers(1, &historyFbos[i]);
            historyFbos[i] = 0;
        }
        if (historyTextures[i] != 0) {
            glDeleteTextures(1, &historyTextures[i]);
            historyTextures[i] = 0;
        }
    }
    historyValid = false;
}

void TAA::begin() const {
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFbo);
}

void TAA::end() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void TAA::apply(const uint32_t depthTexId, const uint32_t velocityTexId,
                const glm::mat4& invViewProj, const glm::mat4& prevViewProj) {
    // Depth testing would discard the full-screen quad behind whatever the
    // scene pass left in the depth buffer.
    glDisable(GL_DEPTH_TEST);

    const uint32_t readIndex = historyIndex;
    historyIndex             = 1U - historyIndex;

    // Pass 1: resolve current + reprojected history into the write slot.
    glBindFramebuffer(GL_FRAMEBUFFER, historyFbos[historyIndex]);

    resolveShader->bind();
    resolveShader->setFloat2("rcpFrame",
                             glm::vec2(1.F / static_cast<float>(width),
                                       1.F / static_cast<float>(height)));
    resolveShader->setFloat("historyBlend", historyValid ? currentWeight : 1.F);
    resolveShader->setMat4("invViewProj", invViewProj);
    resolveShader->setMat4("prevViewProj", prevViewProj);

    glBindTextureUnit(0, sceneColorTexture);
    glBindTextureUnit(1, historyTextures[readIndex]);
    glBindTextureUnit(2, depthTexId);
    glBindTextureUnit(4, velocityTexId);

    quad.draw();
    resolveShader->unbind();

    // Pass 2: present the resolved history. A blit would skip the dither the
    // rest of the pipeline applies when writing an 8-bit target.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    presentShader->bind();
    // Unit 1: the present pass shares historyTex with the resolve pass.
    glBindTextureUnit(1, historyTextures[historyIndex]);
    quad.draw();
    presentShader->unbind();

    historyValid = true;

    glEnable(GL_DEPTH_TEST);
}

void TAA::resize(const uint32_t newWidth, const uint32_t newHeight) {
    if (width == newWidth && height == newHeight) {
        return;
    }
    width  = newWidth;
    height = newHeight;
    destroyFramebuffers();
    createFramebuffers();
}

}  // namespace sponge::platform::opengl::scene
