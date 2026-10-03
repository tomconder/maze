#include "platform/opengl/scene/shadowmap.hpp"

#include "logging/log.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"
#include "platform/opengl/renderer/gl.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <glm/glm.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;

inline const std::string ShadowMap::shaderName = "shadowmap_evsm";

// Moments of a caster at the far plane (depth 1): nothing in front of any
// receiver, so every lookup reads as lit.
static const std::array<float, 4> farMoments = {
    std::exp(ShadowMap::evsmExponent), std::exp(2.F * ShadowMap::evsmExponent),
    -std::exp(-ShadowMap::evsmExponent),
    std::exp(-2.F * ShadowMap::evsmExponent)
};

ShadowMap::ShadowMap(const uint32_t res) :
    shadowHeight(std::min(res, maxResolution)),
    shadowWidth(std::min(res, maxResolution)) {
    initialize();
}

ShadowMap::~ShadowMap() {
    glDeleteFramebuffers(1, &blurFbo);
    glDeleteFramebuffers(static_cast<GLsizei>(maxCascades), momentFbos.data());
    glDeleteTextures(1, &blurTexture);
    glDeleteTextures(static_cast<GLsizei>(maxCascades), momentViews.data());
    glDeleteTextures(1, &momentArray);
    glDeleteRenderbuffers(1, &depthRbo);
}

void ShadowMap::initialize() {
    // Moment array (RGBA32F): the first two moments of exp(c * depth) in the
    // red and green channels and of -exp(-c * depth) in the blue and alpha
    // channels. One layer per cascade.
    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &momentArray);
    glTextureStorage3D(
        momentArray, 1, GL_RGBA32F, static_cast<GLsizei>(shadowWidth),
        static_cast<GLsizei>(shadowHeight), static_cast<GLsizei>(maxCascades));

    // glTextureView needs a name that has no target yet, so these come from
    // glGenTextures; glCreateTextures would fix the target to 2D too soon.
    glGenTextures(static_cast<GLsizei>(maxCascades), momentViews.data());
    for (uint32_t i = 0; i < maxCascades; i++) {
        glTextureView(momentViews[i], GL_TEXTURE_2D, momentArray, GL_RGBA32F, 0,
                      1, i, 1);
    }

    // A view does not inherit the parent's sampler state, so the array and
    // each view get the same one. Outside a cascade's frustum the moments
    // must read as fully lit, which is what the border colour encodes.
    const auto setSampler = [](const uint32_t texture) {
        glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTextureParameterfv(texture, GL_TEXTURE_BORDER_COLOR,
                             farMoments.data());
    };
    setSampler(momentArray);
    for (const auto view : momentViews) {
        setSampler(view);
    }

    // Blur ping-pong texture (same format)
    blurTexture = renderer::createRenderTarget(shadowWidth, shadowHeight,
                                               GL_RGBA32F, GL_LINEAR);

    // Depth renderbuffer for depth testing during the moment-writing pass,
    // shared by every cascade because only one is drawn at a time
    depthRbo = renderer::createDepthRenderbuffer(shadowWidth, shadowHeight);

    // Moment FBOs: colour = one layer of momentArray, depth = depthRbo
    glCreateFramebuffers(static_cast<GLsizei>(maxCascades), momentFbos.data());
    for (uint32_t i = 0; i < maxCascades; i++) {
        glNamedFramebufferTextureLayer(momentFbos[i], GL_COLOR_ATTACHMENT0,
                                       momentArray, 0, static_cast<GLint>(i));
        glNamedFramebufferRenderbuffer(momentFbos[i], GL_DEPTH_ATTACHMENT,
                                       GL_RENDERBUFFER, depthRbo);
        if (glCheckNamedFramebufferStatus(momentFbos[i], GL_FRAMEBUFFER) !=
            GL_FRAMEBUFFER_COMPLETE) {
            SPONGE_GL_CRITICAL("EVSM moment framebuffer {} is not complete!",
                               i);
        }
    }

    // Blur FBO: colour = blurTexture (no depth needed)
    blurFbo = renderer::createFramebuffer("EVSM blur", blurTexture);

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

void ShadowMap::applyBlur(const uint32_t cascade) const {
    glDisable(GL_DEPTH_TEST);

    // Downsample pass: read the cascade's layer → write blurTexture
    blurDownShader->bind();
    blurDownShader->setFloat("offset", 1.F);
    glBindFramebuffer(GL_FRAMEBUFFER, blurFbo);
    glBindTextureUnit(0, momentViews[cascade]);
    quad.draw();
    blurDownShader->unbind();

    // Upsample pass: read blurTexture → write the cascade's layer
    blurUpShader->bind();
    blurUpShader->setFloat("offset", 1.F);
    glBindFramebuffer(GL_FRAMEBUFFER, momentFbos[cascade]);
    glBindTextureUnit(0, blurTexture);
    quad.draw();
    blurUpShader->unbind();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEnable(GL_DEPTH_TEST);
}

void ShadowMap::bind(const uint32_t cascade) const {
    glGetIntegerv(GL_VIEWPORT, savedViewport.data());
    glViewport(0, 0, static_cast<GLsizei>(shadowWidth),
               static_cast<GLsizei>(shadowHeight));
    glBindFramebuffer(GL_FRAMEBUFFER, momentFbos[cascade]);
    // The moments are data, not colour: blending would mix them with the
    // clear value and the alpha channel is a moment too.
    savedBlend = glIsEnabled(GL_BLEND) != 0;
    glDisable(GL_BLEND);
    constexpr float farDepth = 1.F;
    glClearNamedFramebufferfv(momentFbos[cascade], GL_COLOR, 0,
                              farMoments.data());
    glClearNamedFramebufferfv(momentFbos[cascade], GL_DEPTH, 0, &farDepth);
}

void ShadowMap::unbind(const uint32_t cascade) const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    applyBlur(cascade);
    if (savedBlend) {
        glEnable(GL_BLEND);
    }
    glViewport(savedViewport[0], savedViewport[1],
               static_cast<GLsizei>(savedViewport[2]),
               static_cast<GLsizei>(savedViewport[3]));
}

void ShadowMap::bindTexture(const uint8_t unit) const {
    glBindTextureUnit(unit, momentArray);
}

uint32_t ShadowMap::getDepthMapTextureId() const {
    return momentArray;
}

uint32_t ShadowMap::getHeight() const {
    return shadowHeight;
}

uint32_t ShadowMap::getWidth() const {
    return shadowWidth;
}

ShadowMap::Matrices
    ShadowMap::fitLightSpace(const glm::vec3&           lightDirection,
                             const sponge::scene::AABB& sceneBounds,
                             const Camera& camera, const uint32_t count,
                             const uint32_t res, const float splitLambda) {
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

    // Fit the ortho box to the scene's bounds as seen from the light, so the
    // frustum always exactly covers the static scene, at whatever size it
    // is, instead of a fixed box sized for one particular scene.
    const auto viewBounds = sponge::scene::transform(sceneBounds, lightView);

    // Guards against grazing-angle edge clipping on geometry sitting exactly
    // on the scene's boundary (e.g. a floor at the exact minimum Y).
    constexpr float padding = 0.5F;
    const float     zNear   = -viewBounds.max.z - padding;
    const float     zFar    = -viewBounds.min.z + padding;

    Matrices matrices{};
    matrices[count - 1] =
        glm::ortho(viewBounds.min.x - padding, viewBounds.max.x + padding,
                   viewBounds.min.y - padding, viewBounds.max.y + padding,
                   zNear, zFar) *
        lightView;

    // Practical split scheme (Zhang 2006): a blend of log and uniform splits
    // over [near, shadowFar]. Shadows past the scene would be wasted, so the
    // far end stops at the scene's diagonal. It does not depend on the camera
    // position, so a cascade's size never changes while the camera moves.
    const float shadowFar = std::min(
        camera.farPlane, glm::length(sceneBounds.max - sceneBounds.min));

    // Eye-centred sphere around the far corners of the camera frustum slice.
    const float k =
        std::sqrt(1.F + camera.aspect * camera.aspect) * camera.tanHalfFovY;
    const auto eyeLs = glm::vec3(lightView * glm::vec4(camera.position, 1.F));

    for (uint32_t i = 0; i + 1 < count; i++) {
        const float t = static_cast<float>(i + 1) / static_cast<float>(count);
        const float logSplit =
            camera.nearPlane * std::pow(shadowFar / camera.nearPlane, t);
        const float uniformSplit =
            camera.nearPlane + (shadowFar - camera.nearPlane) * t;
        const float split = glm::mix(uniformSplit, logSplit, splitLambda);
        const float r     = split * std::sqrt(1.F + k * k);

        // Snap the centre so the texel grid stays fixed in the light's space.
        // Half the width is a whole number of texels for an even resolution,
        // so the box edges land on the grid too.
        const float texel =
            2.F * r / static_cast<float>(std::min(res, maxResolution));
        const float cx = std::floor(eyeLs.x / texel) * texel;
        const float cy = std::floor(eyeLs.y / texel) * texel;
        matrices[i] =
            glm::ortho(cx - r, cx + r, cy - r, cy + r, zNear, zFar) * lightView;
    }
    return matrices;
}
}  // namespace sponge::platform::opengl::scene
