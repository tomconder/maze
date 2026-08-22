#pragma once

#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/renderer/vertexarray.hpp"
#include "platform/opengl/renderer/vertexbuffer.hpp"

#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <memory>
#include <string_view>

namespace sponge::platform::opengl::scene {

// Temporal anti-aliasing. The camera jitters by a sub-pixel offset every
// frame (see haltonJitter()); this class accumulates those samples in a
// ping-pong history buffer, reprojecting the previous frame through the
// velocity buffer and clamping it to the current 3x3 colour box.
class TAA {
public:
    TAA() = delete;
    TAA(uint32_t width, uint32_t height);
    ~TAA();

    TAA(const TAA&)            = delete;
    TAA& operator=(const TAA&) = delete;

    // Sub-pixel camera offset in NDC for frame `index`, from the Halton(2,3)
    // sequence. Update thread: this is the only part of TAA that is not GL.
    static glm::vec2 haltonJitter(uint32_t index, uint32_t width,
                                  uint32_t height);

    void begin() const;
    void end() const;

    // Resolves the scene into the history buffer and presents it.
    // `velocityTexId` supplies per-pixel motion wherever `depthTexId` is not
    // at the far plane; elsewhere the two matrices reproject the camera alone.
    // `bloomTexId` may be 0 when `bloomIntensity` is 0.
    void apply(uint32_t sceneTexId, uint32_t depthTexId, uint32_t velocityTexId,
               uint32_t bloomTexId, float bloomIntensity,
               const glm::mat4& invViewProj, const glm::mat4& prevViewProj);

    uint32_t getSceneTexture() const {
        return sceneColorTexture;
    }

    void resize(uint32_t newWidth, uint32_t newHeight);

    // Drops the accumulated history; the next resolve starts from scratch.
    void invalidateHistory() {
        historyValid = false;
    }

private:
    static constexpr std::string_view resolveShaderName = "taa_resolve";
    static constexpr std::string_view presentShaderName = "taa_present";

    // Weight of the current frame in the accumulation. 0.1 is ~10 frames of
    // convergence — enough to resolve the jitter pattern without visible lag.
    static constexpr float currentWeight = 0.1F;

    std::shared_ptr<renderer::Shader>       resolveShader;
    std::shared_ptr<renderer::Shader>       presentShader;
    std::unique_ptr<renderer::VertexArray>  vao;
    std::unique_ptr<renderer::VertexBuffer> vbo;

    // Raw GL handles, as in FXAA: the Texture class has no colour-buffer
    // creation path and these are rebuilt on every resize.
    uint32_t sceneFbo          = 0;
    uint32_t sceneColorTexture = 0;
    uint32_t sceneDepthRbo     = 0;

    // Ping-pong: historyIndex is the slot written this frame.
    std::array<uint32_t, 2> historyFbos{};
    std::array<uint32_t, 2> historyTextures{};
    uint32_t                historyIndex = 0;

    uint32_t width        = 0;
    uint32_t height       = 0;
    bool     historyValid = false;

    void initialize();
    void createFramebuffers();
    void destroyFramebuffers();
    void renderQuad() const;
};

}  // namespace sponge::platform::opengl::scene
