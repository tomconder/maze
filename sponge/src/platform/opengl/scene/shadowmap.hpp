#pragma once

#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/scene/screenquad.hpp"
#include "scene/frustum.hpp"

#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace sponge::platform::opengl::scene {
class ShadowMap {
public:
    ShadowMap() = delete;
    explicit ShadowMap(uint32_t res);
    ~ShadowMap();

    ShadowMap(const ShadowMap&)            = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;

    void bind() const;
    void unbind() const;

    void bindTexture(uint8_t unit) const;

    std::shared_ptr<renderer::Shader> getShader() const {
        return shader;
    }

    uint32_t getDepthMapTextureId() const;
    uint32_t getHeight() const;
    uint32_t getWidth() const;

    struct LightFit {
        glm::mat4 matrix{ 1.0F };
        // World-space eye of the light's view matrix, for occlusion queries
        // run against this map's own depth (see OcclusionCuller).
        glm::vec3 eye{ 0.0F };
    };

    // Fits the ortho box and near/far to sceneBounds (world space), so the
    // frustum always exactly covers the static scene regardless of its size,
    // instead of a fixed box sized for whatever scene existed when the
    // constants were picked. Static and free of GL state: the update thread
    // calls it while the render thread may be rebuilding the map.
    static LightFit fitLightSpace(const glm::vec3&           lightDirection,
                                  const sponge::scene::AABB& sceneBounds);

private:
    static const std::string          shaderName;
    static constexpr std::string_view blurDownShaderName = "blur_down";
    static constexpr std::string_view blurUpShaderName   = "blur_up";

    std::shared_ptr<renderer::Shader> shader;
    std::shared_ptr<renderer::Shader> blurDownShader;
    std::shared_ptr<renderer::Shader> blurUpShader;
    ScreenQuad                        quad;

    // Moments live in a 2D array so cascades can be added as layers.
    // momentView is a 2D view of layer 0: it is what the shaders and the blur
    // read, so they stay on a plain sampler2D until a second layer exists.
    uint32_t momentArray = 0;
    uint32_t momentView  = 0;
    uint32_t blurTexture = 0;
    uint32_t depthRbo    = 0;
    uint32_t momentFbo   = 0;
    uint32_t blurFbo     = 0;

    mutable std::array<int, 4> savedViewport{};

    uint32_t shadowHeight;
    uint32_t shadowWidth;

    void initialize();
    void applyBlur() const;
};
}  // namespace sponge::platform::opengl::scene
