#pragma once

#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/scene/screenquad.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>

namespace sponge::platform::opengl::scene {

// Screen-space ambient occlusion: hemisphere-kernel sampling against the
// depth prepass's depth + view-space normal targets, followed by a
// depth-aware blur to remove the per-pixel noise the random kernel rotation
// introduces. See ssao.slang for the technique.
class Ssao {
public:
    Ssao() = delete;
    Ssao(uint32_t width, uint32_t height);
    ~Ssao();

    Ssao(const Ssao&)            = delete;
    Ssao& operator=(const Ssao&) = delete;

    // depthTexId/normalTexId are the depth prepass's targets; projection and
    // its inverse must be the same camera projection the prepass rasterized
    // with, or the reconstructed positions won't line up with the depth.
    void process(uint32_t depthTexId, uint32_t normalTexId,
                 const glm::mat4& projection, const glm::mat4& invProjection,
                 float radius) const;

    uint32_t getTexture() const {
        return blurTexture;
    }

    void resize(uint32_t newWidth, uint32_t newHeight);

private:
    std::shared_ptr<renderer::Shader> ssaoShader;
    std::shared_ptr<renderer::Shader> blurShader;
    ScreenQuad                        quad;

    // 4x4 tiling texture of random rotation vectors, breaking up the kernel's
    // sampling pattern so the blur removes noise instead of banding.
    uint32_t noiseTexture = 0;

    uint32_t rawFbo      = 0;
    uint32_t rawTexture  = 0;
    uint32_t blurFbo     = 0;
    uint32_t blurTexture = 0;

    uint32_t width  = 0;
    uint32_t height = 0;

    void initialize();
    void createNoiseTexture();
    void createFramebuffers();
    void destroyFramebuffers();
};

}  // namespace sponge::platform::opengl::scene
