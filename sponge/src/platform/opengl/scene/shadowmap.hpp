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
// A stack of cascades in one array texture. Cascade 0 is the finest and the
// last cascade in use always covers the whole scene.
class ShadowMap {
public:
    // Must match maxCascades in shaders/slang/include/shadows.slang.
    static constexpr uint32_t maxCascades = 4;

    // Largest side of one cascade, whatever resolution is asked for. All
    // maxCascades layers are allocated, so this bounds the map at four layers
    // of RG32F plus the blur target and the depth buffer, about 176 MB.
    static constexpr uint32_t maxResolution = 2048;

    using Matrices = std::array<glm::mat4, maxCascades>;

    struct Camera {
        glm::vec3 position{ 0.F };
        float     nearPlane   = 0.1F;
        float     farPlane    = 100.F;
        float     tanHalfFovY = 1.F;
        float     aspect      = 1.F;
    };

    ShadowMap() = delete;
    explicit ShadowMap(uint32_t res);
    ~ShadowMap();

    ShadowMap(const ShadowMap&)            = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;

    // Draw into one cascade. unbind() blurs the cascade that bind() drew.
    void bind(uint32_t cascade) const;
    void unbind(uint32_t cascade) const;

    // Binds the whole array, for the sampler2DArray in the lit shaders.
    void bindTexture(uint8_t unit) const;

    std::shared_ptr<renderer::Shader> getShader() const {
        return shader;
    }

    uint32_t getDepthMapTextureId() const;
    uint32_t getHeight() const;
    uint32_t getWidth() const;

    // Light-space matrices for `count` cascades (1 to maxCascades). The last
    // one fits sceneBounds exactly, so the frustum always covers the static
    // scene whatever its size. The nearer ones are boxes around the camera
    // eye, sized by the PSSM split distances (`splitLambda` blends the log
    // split, 1, with the uniform one, 0) and snapped to whole texels of a
    // `res` map, so they change only when the camera moves a texel. Anchoring
    // at the eye, not at the middle of the camera frustum slice, keeps them
    // fixed while the camera only turns. All cascades share one light view
    // and one depth range, so their depths compare. Static and free of GL
    // state: the update thread calls it while the render thread may be
    // rebuilding the map.
    static Matrices fitLightSpace(const glm::vec3&           lightDirection,
                                  const sponge::scene::AABB& sceneBounds,
                                  const Camera& camera, uint32_t count,
                                  uint32_t res, float splitLambda);

private:
    static const std::string          shaderName;
    static constexpr std::string_view blurDownShaderName = "blur_down";
    static constexpr std::string_view blurUpShaderName   = "blur_up";

    std::shared_ptr<renderer::Shader> shader;
    std::shared_ptr<renderer::Shader> blurDownShader;
    std::shared_ptr<renderer::Shader> blurUpShader;
    ScreenQuad                        quad;

    // momentViews[i] is a 2D view of layer i: the blur reads it as a plain
    // sampler2D. The lit shaders sample momentArray itself.
    uint32_t                          momentArray = 0;
    std::array<uint32_t, maxCascades> momentViews{};
    std::array<uint32_t, maxCascades> momentFbos{};
    uint32_t                          blurTexture = 0;
    uint32_t                          depthRbo    = 0;
    uint32_t                          blurFbo     = 0;

    mutable std::array<int, 4> savedViewport{};

    uint32_t shadowHeight;
    uint32_t shadowWidth;

    void initialize();
    void applyBlur(uint32_t cascade) const;
};
}  // namespace sponge::platform::opengl::scene
