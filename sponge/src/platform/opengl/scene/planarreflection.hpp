#pragma once

#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <optional>

namespace sponge::platform::opengl::scene {

// The scene seen in a flat mirror: an HDR color target and a depth buffer at
// the window size, drawn from a camera reflected in the mirror plane.
class PlanarReflection {
public:
    PlanarReflection(uint32_t width, uint32_t height);
    ~PlanarReflection();

    PlanarReflection(const PlanarReflection&)            = delete;
    PlanarReflection& operator=(const PlanarReflection&) = delete;

    void resize(uint32_t newWidth, uint32_t newHeight);

    // Binds and clears the target, saves the caller's viewport and sets this
    // target's own, and flips the front face: a reflection reverses the
    // triangle winding. end() restores the saved viewport and front face, and
    // binds framebuffer 0.
    void begin() const;
    void end() const;

    uint32_t getTexture() const {
        return colorTexture;
    }

    // Reflects points in the plane (n, d), where dot(n, p) + d = 0 on the
    // plane and n has unit length.
    static glm::mat4 reflectionMatrix(const glm::vec4& plane);

    // Moves the near plane of an OpenGL projection onto viewPlane (Lengyel,
    // "Oblique View Frustum Depth Projection and Clipping"). viewPlane is in
    // view space, with the kept side positive. Empty when the frustum corner
    // opposite the plane is not on its kept side.
    static std::optional<glm::mat4>
        obliqueProjection(const glm::mat4& projection,
                          const glm::vec4& viewPlane);

private:
    void create();
    void destroy();

    uint32_t width;
    uint32_t height;
    uint32_t fbo          = 0;
    uint32_t colorTexture = 0;
    uint32_t depthRbo     = 0;
    // Caller's viewport, saved in begin() and restored in end().
    mutable std::array<int32_t, 4> savedViewport{};
};

}  // namespace sponge::platform::opengl::scene
