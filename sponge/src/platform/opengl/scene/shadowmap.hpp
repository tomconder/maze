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
    static constexpr uint32_t SHADOW_WIDTH = 2048;
    static constexpr uint32_t SHADOW_HEIGHT = 2048;

    ShadowMap();
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

    void updateLightSpaceMatrix(const glm::vec3& lightDirection);

private:
    static constexpr char shaderName[] = "shadowmap";
    std::shared_ptr<renderer::Shader> shader;
    std::shared_ptr<renderer::Texture> depthMap;
    std::unique_ptr<renderer::FrameBuffer> framebuffer;

    static constexpr float nearPlane = 1.0f;
    static constexpr float farPlane = 25.0f;

    glm::mat4 lightSpaceMatrix{ 1.0f };
};
}  // namespace sponge::platform::opengl::scene
