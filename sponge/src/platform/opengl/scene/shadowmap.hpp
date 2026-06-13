#pragma once

#include "platform/opengl/renderer/shader.hpp"

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

    void activateAndBindShadowTexture(uint8_t unit) const;

    static std::string_view getShaderName() {
        return shaderName;
    }

    uint32_t getDepthMapTextureId() const;
    uint32_t getHeight() const;

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
    static constexpr std::string_view blurShaderName = "blur";

    std::shared_ptr<renderer::Shader> shader;
    std::shared_ptr<renderer::Shader> blurShader;

    uint32_t momentTexture = 0;
    uint32_t blurTexture   = 0;
    uint32_t depthRbo      = 0;
    uint32_t momentFbo     = 0;
    uint32_t blurFbo       = 0;
    uint32_t blurVao       = 0;
    uint32_t blurVbo       = 0;

    mutable std::array<int, 4> savedViewport{};

    float    orthoSize;
    uint32_t shadowHeight;
    uint32_t shadowWidth;
    float    zFar;
    float    zNear;

    glm::mat4 lightSpaceMatrix{ 1.0f };

    void initialize();
    void applyBlur() const;
};
}  // namespace sponge::platform::opengl::scene
