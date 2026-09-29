#include "platform/opengl/scene/shadowmap.hpp"

#include "logging/log.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"
#include "platform/opengl/renderer/gl.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <glm/glm.hpp>
#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;

inline const std::string ShadowMap::shaderName = "shadowmap_evsm";

ShadowMap::ShadowMap(const uint32_t res) : shadowHeight(res), shadowWidth(res) {
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
    momentTexture = renderer::createRenderTarget(shadowWidth, shadowHeight,
                                                 GL_RG32F, GL_LINEAR);
    // Overrides the helper's CLAMP_TO_EDGE: outside the light frustum the
    // moments must read as fully lit, which is what the border colour encodes.
    glTextureParameteri(momentTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTextureParameteri(momentTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    constexpr std::array borderColor = { 1.F, 1.F, 0.F, 0.F };
    glTextureParameterfv(momentTexture, GL_TEXTURE_BORDER_COLOR,
                         borderColor.data());

    // Blur ping-pong texture (same format)
    blurTexture = renderer::createRenderTarget(shadowWidth, shadowHeight,
                                               GL_RG32F, GL_LINEAR);

    // Depth renderbuffer for depth testing during the moment-writing pass
    depthRbo = renderer::createDepthRenderbuffer(shadowWidth, shadowHeight);

    // Moment FBO: colour = momentTexture, depth = depthRbo
    momentFbo =
        renderer::createFramebuffer("EVSM moment", momentTexture, depthRbo);

    // Blur FBO: colour = blurTexture (no depth needed)
    blurFbo = renderer::createFramebuffer("EVSM blur", blurTexture);

    // Fullscreen quad VAO/VBO for blur pass
    constexpr std::array quadVerts = {
        -1.F, 1.F, 0.F, 1.F, -1.F, -1.F, 0.F, 0.F, 1.F, -1.F, 1.F, 0.F,
        -1.F, 1.F, 0.F, 1.F, 1.F,  -1.F, 1.F, 0.F, 1.F, 1.F,  1.F, 1.F,
    };
    glCreateBuffers(1, &blurVbo);
    glNamedBufferStorage(blurVbo, sizeof(quadVerts), quadVerts.data(), 0);
    glCreateVertexArrays(1, &blurVao);
    glVertexArrayVertexBuffer(blurVao, 0, blurVbo, 0, 4 * sizeof(float));
    glEnableVertexArrayAttrib(blurVao, 0);
    glVertexArrayAttribFormat(blurVao, 0, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(blurVao, 0, 0);
    glEnableVertexArrayAttrib(blurVao, 1);
    glVertexArrayAttribFormat(blurVao, 1, 2, GL_FLOAT, GL_FALSE,
                              2 * sizeof(float));
    glVertexArrayAttribBinding(blurVao, 1, 0);

    // EVSM moment-writing shader (reuses shadowmap.vert)
    const auto shaderCreateInfo = renderer::ShaderCreateInfo{
        .name           = shaderName,
        .vertexShader   = "shadowmap.vert",
        .fragmentShader = "shadowmap_evsm.frag",
    };
    shader = AssetManager::createShader(shaderCreateInfo);

    // Dual Kawase downsample shader
    const auto blurDownShaderInfo = renderer::ShaderCreateInfo{
        .name           = std::string(blurDownShaderName),
        .vertexShader   = "screenquad.vert",
        .fragmentShader = "blur.frag",
    };
    blurDownShader = AssetManager::createShader(blurDownShaderInfo);

    // Dual Kawase upsample shader
    const auto blurUpShaderInfo = renderer::ShaderCreateInfo{
        .name           = std::string(blurUpShaderName),
        .vertexShader   = "screenquad.vert",
        .fragmentShader = "blur_up.frag",
    };
    blurUpShader = AssetManager::createShader(blurUpShaderInfo);
}

void ShadowMap::applyBlur() const {
    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(blurVao);

    // Downsample pass: read momentTexture → write blurTexture
    blurDownShader->bind();
    blurDownShader->setFloat("offset", 1.F);
    glBindFramebuffer(GL_FRAMEBUFFER, blurFbo);
    glBindTextureUnit(0, momentTexture);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    blurDownShader->unbind();

    // Upsample pass: read blurTexture → write momentTexture
    blurUpShader->bind();
    blurUpShader->setFloat("offset", 1.F);
    glBindFramebuffer(GL_FRAMEBUFFER, momentFbo);
    glBindTextureUnit(0, blurTexture);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    blurUpShader->unbind();

    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
    glBindTextureUnit(unit, momentTexture);
}

uint32_t ShadowMap::getDepthMapTextureId() const {
    return momentTexture;
}

uint32_t ShadowMap::getHeight() const {
    return shadowHeight;
}

uint32_t ShadowMap::getWidth() const {
    return shadowWidth;
}

const glm::mat4& ShadowMap::getLightSpaceMatrix() const {
    return lightSpaceMatrix;
}

void ShadowMap::updateLightSpaceMatrix(const glm::vec3& lightDirection,
                                       const sponge::scene::AABB& sceneBounds) {
    const auto center = (sceneBounds.min + sceneBounds.max) * 0.5F;
    // Half the AABB's diagonal: the farthest any corner sits from center, so
    // placing the eye two of these out along -lightDirection clears every
    // corner with room to spare, however the scene is shaped.
    const auto radius = glm::length(sceneBounds.max - sceneBounds.min) * 0.5F;

    // avoid lookAt degenerating when light direction nears vertical
    const auto up        = std::abs(lightDirection.y) > 0.99F ?
                               glm::vec3(0.F, 0.F, 1.F) :
                               glm::vec3(0.F, 1.F, 0.F);
    const auto eye       = center - lightDirection * radius * 2.F;
    const auto lightView = glm::lookAt(eye, center, up);
    eyePosition          = eye;

    // Fit the ortho box to the scene's bounds as seen from the light, so the
    // frustum always exactly covers the static scene, at whatever size it
    // is, instead of a fixed box sized for one particular scene.
    const auto viewBounds = sponge::scene::transform(sceneBounds, lightView);

    // Guards against grazing-angle edge clipping on geometry sitting exactly
    // on the scene's boundary (e.g. a floor at the exact minimum Y).
    constexpr float padding = 0.5F;
    const auto      lightProjection =
        glm::ortho(viewBounds.min.x - padding, viewBounds.max.x + padding,
                   viewBounds.min.y - padding, viewBounds.max.y + padding,
                   -viewBounds.max.z - padding, -viewBounds.min.z + padding);

    lightSpaceMatrix = lightProjection * lightView;
}
}  // namespace sponge::platform::opengl::scene
