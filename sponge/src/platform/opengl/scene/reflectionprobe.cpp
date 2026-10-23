#include "platform/opengl/scene/reflectionprobe.hpp"

#include "logging/log.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"
#include "platform/opengl/renderer/gl.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <bit>

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;

namespace {
// The background when there is no environment image: a flat grey, in linear
// radiance. A tinted background would tint every shadow, so keep it grey.
constexpr std::array<float, 4> flatSky{ .6F, .6F, .6F, 1.F };

// Immutable storage for every face at every level.
uint32_t createCube(const int levels, const uint32_t size) {
    uint32_t id = 0;
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &id);
    // RGBA16F, not RGB16F: RGB16F is not a required color-renderable format
    // in GL 4.5 core, and both cubes are FBO attachments that
    // glGenerateTextureMipmap must be able to render into.
    glTextureStorage2D(id, levels, GL_RGBA16F, static_cast<GLsizei>(size),
                       static_cast<GLsizei>(size));
    glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    return id;
}
}  // namespace

ReflectionProbe::ReflectionProbe(const std::string& environmentPath) {
    // Filter across face edges, so rough mips show no seams.
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    // The capture keeps a full mip chain: the prefilter reads coarser levels
    // for wide lobes, which removes the noise of 512 samples.
    captureCube          = createCube(std::bit_width(faceSize), faceSize);
    resultCube           = createCube(mipLevels, faceSize);
    bounceCube           = createCube(std::bit_width(faceSize), faceSize);
    bounceIrradianceCube = createCube(1, irradianceSize);
    skyCube              = createCube(std::bit_width(faceSize), faceSize);
    skyIrradianceCube    = createCube(1, irradianceSize);

    glCreateRenderbuffers(1, &depthRbo);
    glNamedRenderbufferStorage(depthRbo, GL_DEPTH_COMPONENT24,
                               static_cast<GLsizei>(faceSize),
                               static_cast<GLsizei>(faceSize));

    glCreateFramebuffers(1, &fbo);

    // Nearest and clamped: a blend of a roof and the ground is no surface.
    glCreateTextures(GL_TEXTURE_2D, 1, &skyHeightTexture);
    glTextureStorage2D(skyHeightTexture, 1, GL_R32F,
                       static_cast<GLsizei>(skyHeightSize),
                       static_cast<GLsizei>(skyHeightSize));
    glTextureParameteri(skyHeightTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(skyHeightTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(skyHeightTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(skyHeightTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glCreateRenderbuffers(1, &skyHeightDepth);
    glNamedRenderbufferStorage(skyHeightDepth, GL_DEPTH_COMPONENT24,
                               static_cast<GLsizei>(skyHeightSize),
                               static_cast<GLsizei>(skyHeightSize));
    glCreateFramebuffers(1, &skyHeightFbo);
    glNamedFramebufferTexture(skyHeightFbo, GL_COLOR_ATTACHMENT0,
                              skyHeightTexture, 0);
    glNamedFramebufferRenderbuffer(skyHeightFbo, GL_DEPTH_ATTACHMENT,
                                   GL_RENDERBUFFER, skyHeightDepth);
    skyHeightShader = AssetManager::createShader(renderer::ShaderCreateInfo{
        .name           = "skyheight",
        .vertexShader   = "skyheight.vert",
        .fragmentShader = "skyheight.frag",
    });

    shader           = AssetManager::createShader(renderer::ShaderCreateInfo{
        .name           = "probe_prefilter",
        .vertexShader   = "screenquad.vert",
        .fragmentShader = "probe_prefilter.frag",
    });
    irradianceShader = AssetManager::createShader(renderer::ShaderCreateInfo{
        .name           = "probe_irradiance",
        .vertexShader   = "screenquad.vert",
        .fragmentShader = "probe_irradiance.frag",
    });

    if (!environmentPath.empty()) {
        environment = AssetManager::createTexture(renderer::TextureCreateInfo{
            .name = "probe_environment",
            .path = environmentPath,
        });
        // A file that fails to load leaves a texture with no storage, which
        // samples as black. Fall back to the flat grey instead.
        if (environment->getWidth() == 0) {
            SPONGE_GL_WARN("Probe environment {} did not load; flat sky",
                           environmentPath);
            environment.reset();
        }
    }
    if (environment) {
        skyShader = AssetManager::createShader(renderer::ShaderCreateInfo{
            .name           = "probe_sky",
            .vertexShader   = "screenquad.vert",
            .fragmentShader = "probe_sky.frag",
        });
    }
}

ReflectionProbe::~ReflectionProbe() {
    glDeleteFramebuffers(1, &fbo);
    glDeleteFramebuffers(1, &skyHeightFbo);
    glDeleteRenderbuffers(1, &skyHeightDepth);
    glDeleteTextures(1, &skyHeightTexture);
    glDeleteRenderbuffers(1, &depthRbo);
    glDeleteTextures(1, &captureCube);
    glDeleteTextures(1, &resultCube);
    glDeleteTextures(1, &bounceCube);
    glDeleteTextures(1, &bounceIrradianceCube);
    glDeleteTextures(1, &skyCube);
    glDeleteTextures(1, &skyIrradianceCube);
}

void ReflectionProbe::beginFace(const int face, const Capture capture) const {
    const bool withSky = capture != Capture::Bounce;
    glGetIntegerv(GL_VIEWPORT, savedViewport.data());
    // A cube map face is layer `face` of the texture (GL 4.5).
    uint32_t target = captureCube;
    if (capture == Capture::Bounce) {
        target = bounceCube;
    } else if (capture == Capture::Sky) {
        target = skyCube;
    }
    glNamedFramebufferTextureLayer(fbo, GL_COLOR_ATTACHMENT0, target, 0, face);
    glNamedFramebufferRenderbuffer(fbo, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                                   depthRbo);
    if (glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) !=
        GL_FRAMEBUFFER_COMPLETE) {
        SPONGE_GL_CRITICAL("Reflection probe framebuffer is not complete!");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, static_cast<GLsizei>(faceSize),
               static_cast<GLsizei>(faceSize));
    glDepthMask(GL_TRUE);
    // Not glClear: the global clear colour is grey, and the sky is data.
    constexpr std::array<float, 4> black{ 0.F, 0.F, 0.F, 1.F };
    glClearNamedFramebufferfv(fbo, GL_COLOR, 0,
                              withSky ? flatSky.data() : black.data());
    const float depth = 1.F;
    glClearNamedFramebufferfv(fbo, GL_DEPTH, 0, &depth);

    if (environment && withSky) {
        // After the clears: a clear with a program bound recompiles it on
        // NVIDIA. The image goes in behind the scene, with no depth test, so
        // it neither writes depth nor hides anything drawn after it.
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        skyShader->bind();
        environment->bind(0);
        skyShader->setInteger("face", face);
        quad.draw();
        skyShader->unbind();
        glBindTextureUnit(0, 0);
        glEnable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }
}

glm::mat4 ReflectionProbe::skyHeightViewProj(const glm::vec3& boxMin,
                                             const glm::vec3& boxMax,
                                             const float      minY,
                                             const float      maxY) {
    const auto size =
        glm::vec3(boxMax.x - boxMin.x, maxY - minY, boxMax.z - boxMin.z);
    // x to x, z to y (flipped, so row 0 is boxMax.z and a front face keeps
    // its winding seen from above), and a higher y to a nearer depth. glm
    // indexes [column][row].
    glm::mat4 m(0.F);
    m[0][0] = 2.F / size.x;
    m[3][0] = -2.F * boxMin.x / size.x - 1.F;
    m[2][1] = -2.F / size.z;
    m[3][1] = 2.F * boxMin.z / size.z + 1.F;
    m[1][2] = -2.F / size.y;
    m[3][2] = 2.F * maxY / size.y - 1.F;
    m[3][3] = 1.F;
    return m;
}

void ReflectionProbe::beginSkyHeight(const float minY) const {
    glGetIntegerv(GL_VIEWPORT, savedViewport.data());
    glBindFramebuffer(GL_FRAMEBUFFER, skyHeightFbo);
    glViewport(0, 0, static_cast<GLsizei>(skyHeightSize),
               static_cast<GLsizei>(skyHeightSize));
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    // Not glClear: the map is data. Below the box floor means no surface.
    const float ground = minY - 1.F;
    glClearNamedFramebufferfv(skyHeightFbo, GL_COLOR, 0, &ground);
    const float depth = 1.F;
    glClearNamedFramebufferfv(skyHeightFbo, GL_DEPTH, 0, &depth);
}

void ReflectionProbe::end() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(savedViewport[0], savedViewport[1], savedViewport[2],
               savedViewport[3]);
}

void ReflectionProbe::prefilter() const {
    glGenerateTextureMipmap(captureCube);
    glGenerateTextureMipmap(bounceCube);
    glGenerateTextureMipmap(skyCube);

    // Roughness 0 is a mirror: mip 0 is an exact copy of the capture.
    glCopyImageSubData(captureCube, GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0, resultCube,
                       GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
                       static_cast<GLsizei>(faceSize),
                       static_cast<GLsizei>(faceSize), 6);

    glGetIntegerv(GL_VIEWPORT, savedViewport.data());
    // Color only: detach the depth buffer, and do not test or blend.
    glNamedFramebufferRenderbuffer(fbo, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                                   0);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    shader->bind();
    glBindTextureUnit(0, captureCube);
    shader->setFloat("faceSize", static_cast<float>(faceSize));
    for (int level = 1; level < mipLevels; level++) {
        const auto size = static_cast<GLsizei>(faceSize >> level);
        glViewport(0, 0, size, size);
        shader->setFloat("roughness", static_cast<float>(level) /
                                          static_cast<float>(mipLevels - 1));
        for (int face = 0; face < 6; face++) {
            glNamedFramebufferTextureLayer(fbo, GL_COLOR_ATTACHMENT0,
                                           resultCube, level, face);
            if (level == 1 && face == 0 &&
                glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) !=
                    GL_FRAMEBUFFER_COMPLETE) {
                SPONGE_GL_CRITICAL(
                    "Reflection probe prefilter framebuffer is not "
                    "complete!");
            }
            shader->setInteger("face", face);
            quad.draw();
        }
    }
    shader->unbind();

    convolveIrradiance(bounceCube, bounceIrradianceCube);
    convolveIrradiance(skyCube, skyIrradianceCube);

    glBindTextureUnit(0, 0);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    end();
}

void ReflectionProbe::convolveIrradiance(const uint32_t source,
                                         const uint32_t target) const {
    glBindTextureUnit(0, source);
    irradianceShader->bind();
    irradianceShader->setFloat("faceSize", static_cast<float>(faceSize));
    glViewport(0, 0, static_cast<GLsizei>(irradianceSize),
               static_cast<GLsizei>(irradianceSize));
    for (int face = 0; face < 6; face++) {
        glNamedFramebufferTextureLayer(fbo, GL_COLOR_ATTACHMENT0, target, 0,
                                       face);
        if (face == 0 && glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) !=
                             GL_FRAMEBUFFER_COMPLETE) {
            SPONGE_GL_CRITICAL(
                "Reflection probe irradiance framebuffer is not complete!");
        }
        irradianceShader->setInteger("face", face);
        quad.draw();
    }
    irradianceShader->unbind();
}

glm::mat4 ReflectionProbe::faceView(const glm::vec3& position, const int face) {
    static constexpr std::array<glm::vec3, 6> forward{
        glm::vec3(1.F, 0.F, 0.F), glm::vec3(-1.F, 0.F, 0.F),
        glm::vec3(0.F, 1.F, 0.F), glm::vec3(0.F, -1.F, 0.F),
        glm::vec3(0.F, 0.F, 1.F), glm::vec3(0.F, 0.F, -1.F),
    };
    static constexpr std::array<glm::vec3, 6> up{
        glm::vec3(0.F, -1.F, 0.F), glm::vec3(0.F, -1.F, 0.F),
        glm::vec3(0.F, 0.F, 1.F),  glm::vec3(0.F, 0.F, -1.F),
        glm::vec3(0.F, -1.F, 0.F), glm::vec3(0.F, -1.F, 0.F),
    };
    return glm::lookAt(position, position + forward[face], up[face]);
}

}  // namespace sponge::platform::opengl::scene
