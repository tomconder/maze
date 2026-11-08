#include "platform/opengl/scene/pointshadowmap.hpp"

#include "logging/log.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"
#include "platform/opengl/renderer/gl.hpp"
#include "platform/opengl/scene/reflectionprobe.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/glm.hpp>

#include <cstdint>

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;

PointShadowMap::PointShadowMap() {
    // The lookup filters across face edges, so a lit surface near an edge of
    // the cube reads no seam.
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    // One cube per light; the layer of face f of light l is l * 6 + f.
    glCreateTextures(GL_TEXTURE_CUBE_MAP_ARRAY, 1, &cubeArray);
    glTextureStorage3D(cubeArray, 1, GL_R16F, static_cast<GLsizei>(faceSize),
                       static_cast<GLsizei>(faceSize),
                       static_cast<GLsizei>(maxLights * 6));
    // The values are distances to compare, not colour: nearest keeps a texel
    // from mixing a near and a far surface.
    glTextureParameteri(cubeArray, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(cubeArray, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(cubeArray, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(cubeArray, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(cubeArray, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // One depth buffer for every face, because only one is drawn at a time.
    depthRbo = renderer::createDepthRenderbuffer(faceSize, faceSize);

    glCreateFramebuffers(1, &fbo);
    glNamedFramebufferRenderbuffer(fbo, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                                   depthRbo);
    glNamedFramebufferTextureLayer(fbo, GL_COLOR_ATTACHMENT0, cubeArray, 0, 0);
    if (glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) !=
        GL_FRAMEBUFFER_COMPLETE) {
        SPONGE_GL_CRITICAL("Point shadow framebuffer is not complete!");
    }

    shader = AssetManager::createShader({
        .name           = "pointshadow",
        .vertexShader   = "pointshadow.vert",
        .fragmentShader = "pointshadow.frag",
    });
    // Same vertex stage; the alpha test is its own program so the discard
    // does not turn off early-Z for the opaque meshes.
    maskedShader = AssetManager::createShader({
        .name           = "pointshadow_masked",
        .vertexShader   = "pointshadow.vert",
        .fragmentShader = "pointshadow_masked.frag",
    });
}

PointShadowMap::~PointShadowMap() {
    glDeleteFramebuffers(1, &fbo);
    glDeleteRenderbuffers(1, &depthRbo);
    glDeleteTextures(1, &cubeArray);
}

void PointShadowMap::bind(const uint32_t light, const uint32_t face) const {
    glGetIntegerv(GL_VIEWPORT, savedViewport.data());
    glViewport(0, 0, static_cast<GLsizei>(faceSize),
               static_cast<GLsizei>(faceSize));
    glNamedFramebufferTextureLayer(fbo, GL_COLOR_ATTACHMENT0, cubeArray, 0,
                                   static_cast<GLint>(light * 6 + face));
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    // The values are data: blending would mix them with the clear value.
    savedBlend = glIsEnabled(GL_BLEND) != 0;
    glDisable(GL_BLEND);
    // Cleared to the edge of the light's reach, so an empty direction reads
    // as lit. Not glClear, whose colour is the global grey.
    constexpr std::array<float, 4> farDistance = { 1.F, 1.F, 1.F, 1.F };
    constexpr float                farDepth    = 1.F;
    glClearNamedFramebufferfv(fbo, GL_COLOR, 0, farDistance.data());
    glClearNamedFramebufferfv(fbo, GL_DEPTH, 0, &farDepth);
}

void PointShadowMap::unbind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (savedBlend) {
        glEnable(GL_BLEND);
    }
    glViewport(savedViewport[0], savedViewport[1],
               static_cast<GLsizei>(savedViewport[2]),
               static_cast<GLsizei>(savedViewport[3]));
}

void PointShadowMap::bindTexture(const uint8_t unit) const {
    glBindTextureUnit(unit, cubeArray);
}

glm::mat4 PointShadowMap::faceViewProj(const glm::vec3& position,
                                       const uint32_t face, const float range) {
    // A 90 degree square view per face, in the orientation the cube map
    // lookup expects (the probe capture draws its faces the same way).
    return glm::perspective(glm::radians(90.F), 1.F, nearPlane, range) *
           ReflectionProbe::faceView(position, static_cast<int>(face));
}
}  // namespace sponge::platform::opengl::scene
