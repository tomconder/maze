#pragma once

#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/scene/screenquad.hpp"

#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <memory>

namespace sponge::platform::opengl::scene {

// The scene seen from one point, as a cubemap. The caller draws each face
// between beginFace() and end(), then prefilter() convolves the capture with
// GGX for every roughness level. Surfaces sample getTexture() at mip
// roughness * (mipLevels - 1).
class ReflectionProbe {
public:
    static constexpr uint32_t faceSize  = 256;
    static constexpr int      mipLevels = 6;

    ReflectionProbe();
    ~ReflectionProbe();

    ReflectionProbe(const ReflectionProbe&)            = delete;
    ReflectionProbe& operator=(const ReflectionProbe&) = delete;

    // Binds face (0..5, GL order +X -X +Y -Y +Z -Z) of the capture cubemap,
    // clears it, and saves the caller's viewport. end() binds framebuffer 0
    // and restores the viewport.
    void beginFace(int face) const;
    void end() const;

    // Fills every mip level of the result cubemap from the capture. Call
    // after all six faces are drawn.
    void prefilter() const;

    uint32_t getTexture() const {
        return resultCube;
    }

    // View matrix for a GL cube face at position, with the up vectors the
    // GL cube map layout expects. Use with a 90 degree, aspect 1 projection.
    static glm::mat4 faceView(const glm::vec3& position, int face);

private:
    uint32_t                          fbo         = 0;
    uint32_t                          depthRbo    = 0;
    uint32_t                          captureCube = 0;
    uint32_t                          resultCube  = 0;
    std::shared_ptr<renderer::Shader> shader;
    ScreenQuad                        quad;
    mutable std::array<int32_t, 4>    savedViewport{};
};

}  // namespace sponge::platform::opengl::scene
