#include "platform/opengl/scene/bloom.hpp"

#include "logging/log.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"
#include "platform/opengl/renderer/gl.hpp"

#include <array>

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

Bloom::Bloom(const uint32_t width, const uint32_t height) :
    width(width), height(height) {
    initialize();
}

Bloom::~Bloom() {
    destroyFramebuffers();
}

void Bloom::initialize() {
    auto makeShader = [](std::string_view name, const char* frag) {
        return AssetManager::createShader(renderer::ShaderCreateInfo{
            .name               = std::string(name),
            .vertexShaderPath   = "/shaders/glsl/screenquad.vert.glsl",
            .fragmentShaderPath = frag,
        });
    };

    extractShader =
        makeShader(extractShaderName, "/shaders/glsl/bloom_extract.frag.glsl");
    downShader =
        makeShader(downShaderName, "/shaders/glsl/bloom_down.frag.glsl");
    upShader = makeShader(upShaderName, "/shaders/glsl/bloom_up.frag.glsl");
    compositeShader = makeShader(compositeShaderName,
                                 "/shaders/glsl/bloom_composite.frag.glsl");

    vao = renderer::VertexArray::create();
    vao->bind();
    vbo = std::make_unique<renderer::VertexBuffer>(quadVertices.data(),
                                                   sizeof(quadVertices));
    vbo->bind();

    // blur.vert.glsl uses layout(location=0) aPos, layout(location=1)
    // aTexCoords
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, quadStride,
                          reinterpret_cast<const void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, quadStride,
                          reinterpret_cast<const void*>(2 * sizeof(float)));

    vao->unbind();

    createFramebuffers();
}

void Bloom::createFramebuffers() {
    // Scene capture FBO at full resolution (HDR)
    glGenTextures(1, &sceneColorTexture);
    glBindTexture(GL_TEXTURE_2D, sceneColorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, static_cast<GLsizei>(width),
                 static_cast<GLsizei>(height), 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenRenderbuffers(1, &sceneDepthRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, sceneDepthRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
                          static_cast<GLsizei>(width),
                          static_cast<GLsizei>(height));
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glGenFramebuffers(1, &sceneFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           sceneColorTexture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, sceneDepthRbo);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        SPONGE_GL_CRITICAL("Bloom scene framebuffer is not complete!");
    }

    // Mip-chain FBOs: level i at (width >> (i+1)) x (height >> (i+1))
    glGenFramebuffers(numLevels, downFbos.data());
    glGenTextures(numLevels, downTextures.data());
    glGenFramebuffers(numLevels, upFbos.data());
    glGenTextures(numLevels, upTextures.data());

    auto makeMipTex = [](uint32_t tex, GLsizei w, GLsizei h) {
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT,
                     nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
    };

    for (int i = 0; i < numLevels; i++) {
        const auto w = static_cast<GLsizei>(width >> (i + 1));
        const auto h = static_cast<GLsizei>(height >> (i + 1));

        makeMipTex(downTextures[i], w, h);
        glBindFramebuffer(GL_FRAMEBUFFER, downFbos[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, downTextures[i], 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
            GL_FRAMEBUFFER_COMPLETE) {
            SPONGE_GL_CRITICAL("Bloom down framebuffer {} is not complete!", i);
        }

        makeMipTex(upTextures[i], w, h);
        glBindFramebuffer(GL_FRAMEBUFFER, upFbos[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, upTextures[i], 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
            GL_FRAMEBUFFER_COMPLETE) {
            SPONGE_GL_CRITICAL("Bloom up framebuffer {} is not complete!", i);
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Bloom::destroyFramebuffers() {
    if (sceneFbo != 0) {
        glDeleteFramebuffers(1, &sceneFbo);
        sceneFbo = 0;
    }
    if (sceneColorTexture != 0) {
        glDeleteTextures(1, &sceneColorTexture);
        sceneColorTexture = 0;
    }
    if (sceneDepthRbo != 0) {
        glDeleteRenderbuffers(1, &sceneDepthRbo);
        sceneDepthRbo = 0;
    }
    glDeleteFramebuffers(numLevels, downFbos.data());
    glDeleteTextures(numLevels, downTextures.data());
    glDeleteFramebuffers(numLevels, upFbos.data());
    glDeleteTextures(numLevels, upTextures.data());
    downFbos.fill(0);
    downTextures.fill(0);
    upFbos.fill(0);
    upTextures.fill(0);
}

void Bloom::begin() const {
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFbo);
}

void Bloom::end() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Bloom::renderQuad() const {
    vao->bind();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, quadVertexCount);
    vao->unbind();
}

void Bloom::process(const float threshold) const {
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);

    // Extract bright pixels → down[0] at (w/2, h/2)
    glBindFramebuffer(GL_FRAMEBUFFER, downFbos[0]);
    glViewport(0, 0, static_cast<GLsizei>(width >> 1),
               static_cast<GLsizei>(height >> 1));
    extractShader->bind();
    extractShader->setFloat("threshold", threshold);
    glBindTexture(GL_TEXTURE_2D, sceneColorTexture);
    renderQuad();
    extractShader->unbind();

    // Downsample: down[i-1] → down[i]
    downShader->bind();
    downShader->setFloat("offset", 1.0F);
    for (int i = 1; i < numLevels; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, downFbos[i]);
        glViewport(0, 0, static_cast<GLsizei>(width >> (i + 1)),
                   static_cast<GLsizei>(height >> (i + 1)));
        glBindTexture(GL_TEXTURE_2D, downTextures[i - 1]);
        renderQuad();
    }
    downShader->unbind();

    // Upsample: from deepest down level back up to up[0], accumulating each
    // level's downsample so the coarsest mip's texel structure never shows.
    upShader->bind();
    upShader->setFloat("offset", 1.0F);
    for (int i = numLevels - 1; i >= 0; i--) {
        glBindFramebuffer(GL_FRAMEBUFFER, upFbos[i]);
        glViewport(0, 0, static_cast<GLsizei>(width >> (i + 1)),
                   static_cast<GLsizei>(height >> (i + 1)));
        upShader->setFloat("accumulate", i == numLevels - 1 ? 0.F : 1.F);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, downTextures[i]);
        glActiveTexture(GL_TEXTURE0);
        const uint32_t src =
            (i == numLevels - 1) ? downTextures[i] : upTextures[i + 1];
        glBindTexture(GL_TEXTURE_2D, src);
        renderQuad();
    }
    upShader->unbind();
    glActiveTexture(GL_TEXTURE0);

    glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEnable(GL_DEPTH_TEST);
}

void Bloom::apply(const float intensity) const {
    glDisable(GL_DEPTH_TEST);

    compositeShader->bind();
    compositeShader->setFloat("intensity", intensity);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneColorTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, upTextures[0]);

    renderQuad();
    compositeShader->unbind();

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_DEPTH_TEST);
}

void Bloom::resize(const uint32_t newWidth, const uint32_t newHeight) {
    if (width == newWidth && height == newHeight) {
        return;
    }
    width  = newWidth;
    height = newHeight;
    destroyFramebuffers();
    createFramebuffers();
}

}  // namespace sponge::platform::opengl::scene
