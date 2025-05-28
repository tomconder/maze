#pragma once

#include "platform/opengl/renderer/framebuffer.hpp"
#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/renderer/texture.hpp"
#include <glm/ext/matrix_transform.hpp>
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

    static std::string getShaderName() {
        return std::string(shaderName);
    }

    uint32_t getDepthMapId() const {
        return depthMap;
    }
    const glm::mat4& getLightSpaceMatrix() const {
        return lightSpaceMatrix;
    }

    void updateLightSpaceMatrix(const glm::vec3& lightPos);

   private:
    static constexpr char shaderName[] = "shadowmap";
    std::shared_ptr<renderer::Shader> shader;

    static constexpr float nearPlane = 1.0f;
    static constexpr float farPlane = 25.0f;

    uint32_t depthMap{ 0 };
    uint32_t fbo{ 0 };
    glm::mat4 lightSpaceMatrix{ 1.0f };
};
}  // namespace sponge::platform::opengl::scene
