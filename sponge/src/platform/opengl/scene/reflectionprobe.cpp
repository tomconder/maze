#include "platform/opengl/scene/reflectionprobe.hpp"

#include "logging/log.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"
#include "platform/opengl/renderer/gl.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <bit>

namespace sponge::platform::opengl::scene {
using renderer::AssetManager;

namespace {
// Immutable storage for every face at every level.
uint32_t createCube(const int levels) {
    uint32_t id = 0;
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &id);
    // RGBA16F, not RGB16F: RGB16F is not a required color-renderable format
    // in GL 4.5 core, and both cubes are FBO attachments that
    // glGenerateTextureMipmap must be able to render into.
    glTextureStorage2D(id, levels, GL_RGBA16F,
                       static_cast<GLsizei>(ReflectionProbe::faceSize),
                       static_cast<GLsizei>(ReflectionProbe::faceSize));
    glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    return id;
}
}  // namespace

ReflectionProbe::ReflectionProbe() {
    // Filter across face edges, so rough mips show no seams.
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    // The capture keeps a full mip chain: the prefilter reads coarser levels
    // for wide lobes, which removes the noise of 512 samples.
    captureCube = createCube(std::bit_width(faceSize));
    resultCube  = createCube(mipLevels);

    glCreateRenderbuffers(1, &depthRbo);
    glNamedRenderbufferStorage(depthRbo, GL_DEPTH_COMPONENT24,
                               static_cast<GLsizei>(faceSize),
                               static_cast<GLsizei>(faceSize));

    glCreateFramebuffers(1, &fbo);

    shader = AssetManager::createShader(renderer::ShaderCreateInfo{
        .name           = "probe_prefilter",
        .vertexShader   = "screenquad.vert",
        .fragmentShader = "probe_prefilter.frag",
    });
}

ReflectionProbe::~ReflectionProbe() {
    glDeleteFramebuffers(1, &fbo);
    glDeleteRenderbuffers(1, &depthRbo);
    glDeleteTextures(1, &captureCube);
    glDeleteTextures(1, &resultCube);
}

void ReflectionProbe::beginFace(const int face) const {
    glGetIntegerv(GL_VIEWPORT, savedViewport.data());
    // A cube map face is layer `face` of the texture (GL 4.5).
    glNamedFramebufferTextureLayer(fbo, GL_COLOR_ATTACHMENT0, captureCube, 0,
                                   face);
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
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void ReflectionProbe::end() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(savedViewport[0], savedViewport[1], savedViewport[2],
               savedViewport[3]);
}

void ReflectionProbe::prefilter() const {
    glGenerateTextureMipmap(captureCube);

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

    glBindTextureUnit(0, 0);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    end();
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
