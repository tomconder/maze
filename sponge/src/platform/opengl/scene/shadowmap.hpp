#pragma once

#include "platform/opengl/renderer/framebuffer.hpp"
#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/renderer/texture.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <string>

namespace sponge::platform::opengl::scene {
class ShadowMap {
public:
    ShadowMap() = delete;
    explicit ShadowMap(uint32_t res);
    ~ShadowMap() = default;

    void bind() const;
    void unbind() const;

    void activateAndBindDepthMap(uint8_t unit) const;

    static std::string getShaderName() {
        return std::string(shaderName);
    }

    const glm::mat4& getLightSpaceMatrix() const {
        return lightSpaceMatrix;
    }

    uint32_t getDepthMapTextureId() const {
        if (depthMap != nullptr) {
            return depthMap->getId();
        }
        return 0;
    }

    uint32_t getWidth() const {
        return shadowWidth;
    }

    uint32_t getHeight() const {
        return shadowHeight;
    }

    float getZNear() const {
        return zNear;
    }

    void setZNear(const float val) {
        zNear = val;
    }

    float getZFar() const {
        return zFar;
    }

    void setZFar(const float val) {
        zFar = val;
    }

    float getOrthoSize() const {
        return orthoSize;
    }

    void setOrthoSize(const float val) {
        orthoSize = val;
    }

    void updateLightSpaceMatrix(const glm::vec3& lightDirection);

private:
    static constexpr char shaderName[] = "shadowmap";
    std::shared_ptr<renderer::Shader> shader;
    std::shared_ptr<renderer::Texture> depthMap;
    std::unique_ptr<renderer::FrameBuffer> framebuffer;

    float zNear = 0.F;
    float zFar = 1.F;

    float orthoSize;
    uint32_t shadowWidth;
    uint32_t shadowHeight;

    glm::mat4 lightSpaceMatrix{ 1.0f };

    void initialize();
};
}  // namespace sponge::platform::opengl::scene
