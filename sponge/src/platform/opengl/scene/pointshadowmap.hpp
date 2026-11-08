#pragma once

#include "platform/opengl/renderer/shader.hpp"

#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <memory>

namespace sponge::platform::opengl::scene {
// Shadows for the first maxLights point lights: one cube map per light, in one
// cube map array. A texel holds the distance from the light to the nearest
// surface in that direction, divided by the light's range, so 1 is the edge of
// its reach and the clear value reads as lit. Lights past maxLights are not
// shadowed.
class PointShadowMap {
public:
    // Must match maxPointShadows in shaders/slang/include/shadows.slang.
    static constexpr uint32_t maxLights = 8;

    // Side of one cube face. R16F, so the map takes
    // maxLights * 6 * faceSize^2 * 2 bytes, 25 MB, and a depth buffer.
    static constexpr uint32_t faceSize = 512;

    // Nearest distance the faces draw. Closer than this to a light, nothing is
    // seen.
    static constexpr float nearPlane = 0.05F;

    PointShadowMap();
    ~PointShadowMap();

    PointShadowMap(const PointShadowMap&)            = delete;
    PointShadowMap& operator=(const PointShadowMap&) = delete;

    // Draw into one face (0 to 5, in cube map order) of one light's cube.
    // bind() clears the face and unbind() puts the viewport and blend state
    // back.
    void bind(uint32_t light, uint32_t face) const;
    void unbind() const;

    // Binds the whole array, for the samplerCubeArray in the lit shaders.
    void bindTexture(uint8_t unit) const;

    // View-projection of one face for a light at `position` that reaches
    // `range`.
    static glm::mat4 faceViewProj(const glm::vec3& position, uint32_t face,
                                  float range);

    std::shared_ptr<renderer::Shader> getShader() const {
        return shader;
    }
    std::shared_ptr<renderer::Shader> getMaskedShader() const {
        return maskedShader;
    }

private:
    std::shared_ptr<renderer::Shader> shader;
    std::shared_ptr<renderer::Shader> maskedShader;  // alpha-tested meshes

    uint32_t cubeArray = 0;
    uint32_t depthRbo  = 0;
    uint32_t fbo       = 0;

    mutable std::array<int, 4> savedViewport{};
    mutable bool               savedBlend = false;
};
}  // namespace sponge::platform::opengl::scene
