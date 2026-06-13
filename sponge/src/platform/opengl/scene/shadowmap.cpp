#include "platform/opengl/scene/shadowmap.hpp"

#include "logging/log.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"
#include "platform/opengl/renderer/gl.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <array>
#include <memory>

namespace {
constexpr float nearPlane    = 1.F;
constexpr float farPlane     = 75.F;
constexpr float orthoBoxSize = 5.F;
}  // namespace

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;

inline const std::string ShadowMap::shaderName = "shadowmap_evsm";

ShadowMap::ShadowMap(const uint32_t res) :
    orthoSize(orthoBoxSize),
    shadowHeight(res),
    shadowWidth(res),
    zFar(farPlane),
    zNear(nearPlane) {
    initialize();
}

ShadowMap::~ShadowMap() {
    if (blurVao != 0) {
        glDeleteVertexArrays(1, &blurVao);
    }
    if (blurVbo != 0) {
        glDeleteBuffers(1, &blurVbo);
    }
    if (blurFbo != 0) {
        glDeleteFramebuffers(1, &blurFbo);
    }
    if (momentFbo != 0) {
        glDeleteFramebuffers(1, &momentFbo);
    }
    if (blurTexture != 0) {
        glDeleteTextures(1, &blurTexture);
    }
    if (momentTexture != 0) {
        glDeleteTextures(1, &momentTexture);
    }
    if (depthRbo != 0) {
        glDeleteRenderbuffers(1, &depthRbo);
    }
}

void ShadowMap::initialize() {
    // Moment texture (RG32F): R = depth, G = depth²
    glGenTextures(1, &momentTexture);
    glBindTexture(GL_TEXTURE_2D, momentTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, static_cast<GLsizei>(shadowWidth),
                 static_cast<GLsizei>(shadowHeight), 0, GL_RG, GL_FLOAT,
                 nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const std::array<float, 4> borderColor = { 1.F, 1.F, 0.F, 0.F };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR,
                     borderColor.data());

    // Blur ping-pong texture (same format)
    glGenTextures(1, &blurTexture);
    glBindTexture(GL_TEXTURE_2D, blurTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, static_cast<GLsizei>(shadowWidth),
                 static_cast<GLsizei>(shadowHeight), 0, GL_RG, GL_FLOAT,
                 nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Depth renderbuffer for depth testing during the moment-writing pass
    glGenRenderbuffers(1, &depthRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
                          static_cast<GLsizei>(shadowWidth),
                          static_cast<GLsizei>(shadowHeight));

    // Moment FBO: colour = momentTexture, depth = depthRbo
    glGenFramebuffers(1, &momentFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, momentFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           momentTexture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, depthRbo);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        SPONGE_GL_CRITICAL("EVSM moment framebuffer is not complete!");
    }

    // Blur FBO: colour = blurTexture (no depth needed)
    glGenFramebuffers(1, &blurFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, blurFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           blurTexture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        SPONGE_GL_CRITICAL("EVSM blur framebuffer is not complete!");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Fullscreen quad VAO/VBO for blur pass
    constexpr std::array<float, 24> quadVerts = {
        -1.F, 1.F, 0.F, 1.F, -1.F, -1.F, 0.F, 0.F, 1.F, -1.F, 1.F, 0.F,
        -1.F, 1.F, 0.F, 1.F, 1.F,  -1.F, 1.F, 0.F, 1.F, 1.F,  1.F, 1.F
    };
    glGenVertexArrays(1, &blurVao);
    glGenBuffers(1, &blurVbo);
    glBindVertexArray(blurVao);
    glBindBuffer(GL_ARRAY_BUFFER, blurVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts.data(),
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<void*>(2 * sizeof(float)));
    glBindVertexArray(0);

    // EVSM moment-writing shader (reuses shadowmap.vert.glsl)
    const auto shaderCreateInfo = renderer::ShaderCreateInfo{
        .name               = shaderName,
        .vertexShaderPath   = "/shaders/glsl/shadowmap.vert.glsl",
        .fragmentShaderPath = "/shaders/glsl/shadowmap_evsm.frag.glsl",
    };
    shader = AssetManager::createShader(shaderCreateInfo);

    // Gaussian blur shader
    const auto blurShaderInfo = renderer::ShaderCreateInfo{
        .name               = std::string(blurShaderName),
        .vertexShaderPath   = "/shaders/glsl/blur.vert.glsl",
        .fragmentShaderPath = "/shaders/glsl/blur.frag.glsl",
    };
    blurShader = AssetManager::createShader(blurShaderInfo);
}

void ShadowMap::applyBlur() const {
    glDisable(GL_DEPTH_TEST);
    blurShader->bind();
    blurShader->setInteger("image", 0);
    glBindVertexArray(blurVao);
    glActiveTexture(GL_TEXTURE0);

    // Horizontal pass: read momentTexture → write blurTexture
    glBindFramebuffer(GL_FRAMEBUFFER, blurFbo);
    blurShader->setBoolean("horizontal", true);
    glBindTexture(GL_TEXTURE_2D, momentTexture);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Vertical pass: read blurTexture → write momentTexture
    glBindFramebuffer(GL_FRAMEBUFFER, momentFbo);
    blurShader->setBoolean("horizontal", false);
    glBindTexture(GL_TEXTURE_2D, blurTexture);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    blurShader->unbind();
    glEnable(GL_DEPTH_TEST);
}

void ShadowMap::bind() const {
    glGetIntegerv(GL_VIEWPORT, savedViewport.data());
    glViewport(0, 0, static_cast<GLsizei>(shadowWidth),
               static_cast<GLsizei>(shadowHeight));
    glBindFramebuffer(GL_FRAMEBUFFER, momentFbo);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void ShadowMap::unbind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    applyBlur();
    glViewport(savedViewport[0], savedViewport[1],
               static_cast<GLsizei>(savedViewport[2]),
               static_cast<GLsizei>(savedViewport[3]));
}

void ShadowMap::activateAndBindShadowTexture(const uint8_t unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, momentTexture);
}

uint32_t ShadowMap::getDepthMapTextureId() const {
    return momentTexture;
}

uint32_t ShadowMap::getHeight() const {
    return shadowHeight;
}

float ShadowMap::getOrthoSize() const {
    return orthoSize;
}

void ShadowMap::setOrthoSize(const float val) {
    orthoSize = val;
}

uint32_t ShadowMap::getWidth() const {
    return shadowWidth;
}

float ShadowMap::getZFar() const {
    return zFar;
}

void ShadowMap::setZFar(const float val) {
    zFar = val;
}

float ShadowMap::getZNear() const {
    return zNear;
}

void ShadowMap::setZNear(const float val) {
    zNear = val;
}

const glm::mat4& ShadowMap::getLightSpaceMatrix() const {
    return lightSpaceMatrix;
}

void ShadowMap::updateLightSpaceMatrix(const glm::vec3& lightDirection) {
    const float left   = -orthoSize;
    const float right  = orthoSize;
    const float bottom = -orthoSize;
    const float top    = orthoSize;

    const auto lightProjection =
        glm::ortho(left, right, bottom, top, zNear, zFar);

    const auto lightView = glm::lookAt(10.F * -lightDirection, glm::vec3(0.0f),
                                       glm::vec3(0.0f, 1.0f, 0.0f));

    lightSpaceMatrix = lightProjection * lightView;
}
}  // namespace sponge::platform::opengl::scene
