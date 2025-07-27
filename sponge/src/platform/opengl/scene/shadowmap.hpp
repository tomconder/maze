#pragma once

#include "platform/opengl/renderer/framebuffer.hpp"
#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/renderer/texture.hpp"
#include <glm/glm.hpp>
#include <memory>

namespace sponge::platform::opengl::scene {
class ShadowMap {
   public:
    static constexpr uint32_t SHADOW_WIDTH = 1024;
    static constexpr uint32_t SHADOW_HEIGHT = 1024;

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

    void updateLightSpaceMatrix(const glm::vec3& lightPos);

   private:
    static constexpr char shaderName[] = "shadowmap";
    std::shared_ptr<renderer::Shader> shader;
    std::unique_ptr<renderer::FrameBuffer> framebuffer;
    std::unique_ptr<renderer::Texture> depthMap;

    static constexpr float nearPlane = 1.0f;
    static constexpr float farPlane = 25.0f;

    glm::mat4 lightSpaceMatrix{ 1.0f };
};
}  // namespace sponge::platform::opengl::scene
