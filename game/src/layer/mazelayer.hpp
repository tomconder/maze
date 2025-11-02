#pragma once

#include "scene/gamecamera.hpp"
#include "sponge.hpp"

#include <memory>

namespace game::layer {
struct GameObject {
    const char* name;
    const char* path;
    glm::vec3 scale{ 1.F };
    struct {
        float angle{ 0.F };
        glm::vec3 axis{ 0.F, 1.F, 0.F };
    } rotation;
    glm::vec3 translation{ 0.F };
};

class MazeLayer final : public sponge::layer::Layer {
public:
    MazeLayer();

    void onAttach() override;

    void onDetach() override;

    void onEvent(sponge::event::Event& event) override;

    bool onUpdate(double elapsedTime) override;

    std::shared_ptr<scene::GameCamera> getCamera() const {
        return camera;
    }

    bool isMetallic() const {
        return metallic;
    }

    void setMetallic(bool val);

    float getAmbientOcclusion() const {
        return ao;
    }

    void setAmbientOcclusion(float val);

    float getAmbientStrength() const {
        return ambientStrength;
    }

    void setAmbientStrength(float val);

    float getRoughness() const {
        return roughness;
    }

    void setRoughness(float val);

    int32_t getNumLights() const {
        return numLights;
    }

    void setNumLights(int32_t val);

    int32_t getAttenuationIndex() const {
        return attenuationIndex;
    }

    void setAttenuationIndex(int32_t val);

    uint32_t getDepthMapTextureId() const {
        return shadowMap->getDepthMapTextureId();
    }

    float getDepthMapZNear() const {
        return shadowMap->getZNear();
    }

    float getDepthMapZFar() const {
        return shadowMap->getZFar();
    }

    float getDepthMapOrthoBoxSize() const {
        return shadowMap->getOrthoBoxSize();
    }

    bool getDirectionalLightCastsShadow() const;

    void setDirectionalLightCastsShadow(bool value);

    glm::vec3 getDirectionalLightColor() const;

    void setDirectionalLightColor(const glm::vec3& color);

    glm::vec3 getDirectionalLightDirection() const;

    void setDirectionalLightDirection(const glm::vec3& direction);

    bool getDirectionalLightEnabled() const;

    void setDirectionalLightEnabled(bool value);

    float getDirectionalLightShadowBias() const;

    void setDirectionalLightShadowBias(float value);

    uint32_t getDirectionalLightShadowMapRes() const;

private:
    std::shared_ptr<scene::GameCamera> camera;
    std::unique_ptr<sponge::platform::opengl::scene::Cube> cube;
    std::unique_ptr<sponge::platform::opengl::scene::ShadowMap> shadowMap;

    bool metallic = false;
    float ambientStrength = .25F;
    float ao = .25F;
    float roughness = .5F;
    int32_t numLights = 1;
    int32_t attenuationIndex = 4;

    bool onMouseButtonPressed(
        const sponge::event::MouseButtonPressedEvent& event);

    bool onMouseButtonReleased(
        const sponge::event::MouseButtonReleasedEvent& event);

    bool onMouseScrolled(const sponge::event::MouseScrolledEvent& event) const;

    bool onWindowResize(const sponge::event::WindowResizeEvent& event) const;

    void renderGameObjects() const;

    void renderLightCubes() const;

    void renderSceneToDepthMap() const;

    void updateCamera(double elapsedTime) const;

    void updateShaderLights() const;
};
}  // namespace game::layer
