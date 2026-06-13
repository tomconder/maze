#pragma once

#include "platform/opengl/renderer/framebuffer.hpp"
#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/renderer/texture.hpp"
#include "platform/opengl/scene/shadowmode.hpp"

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

    void activateAndBindDepthMap(uint8_t unit) const;
    void activateAndBindShadowTexture(uint8_t unit) const;

    static std::string_view getShaderName() {
        return shaderName;
    }

    static std::string_view getEvsmShaderName() {
        return evsmShaderName;
    }

    uint32_t getDepthMapTextureId() const;

    uint32_t getHeight() const;

    ShadowMode getMode() const;
    void       setMode(ShadowMode mode);

    float getOrthoSize() const;
    void  setOrthoSize(float val);

    uint32_t getWidth() const;

    float getZFar() const;
    void  setZFar(float val);

    float getZNear() const;
    void  setZNear(float val);

    const glm::mat4& getLightSpaceMatrix() const;
    void             updateLightSpaceMatrix(const glm::vec3& lightDirection);

private:
    static const std::string          shaderName;
    static const std::string          evsmShaderName;
    static constexpr std::string_view blurShaderName = "blur";

    std::shared_ptr<renderer::Shader>      shader;
    std::shared_ptr<renderer::Shader>      evsmShader;
    std::shared_ptr<renderer::Shader>      blurShader;
    std::shared_ptr<renderer::Texture>     depthMap;
    std::unique_ptr<renderer::FrameBuffer> framebuffer;

    // Raw GL handles — EVSM moment map and blur ping-pong FBOs.
    // Zero means not allocated.
    uint32_t momentTexture = 0;
    uint32_t blurTexture   = 0;
    uint32_t depthRbo      = 0;
    uint32_t momentFbo     = 0;
    uint32_t blurFbo       = 0;
    uint32_t blurVao       = 0;
    uint32_t blurVbo       = 0;

    mutable std::array<int, 4> savedViewport{};

    ShadowMode shadowMode{ ShadowMode::PCF };
    float      orthoSize;
    uint32_t   shadowHeight;
    uint32_t   shadowWidth;
    float      zFar;
    float      zNear;

    glm::mat4 lightSpaceMatrix{ 1.0f };

    void initialize();
    void initializeEvsm();
    void destroyEvsm();
    void applyBlur() const;
};
}  // namespace sponge::platform::opengl::scene
