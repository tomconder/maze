#pragma once

#include "scene/gamecamera.hpp"
#include "sponge.hpp"

namespace game::layer {

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

    void setMetallic(bool metallic);

    float getAmbientOcclusion() const {
        return ao;
    }

    void setAmbientOcclusion(float ao);

    float getAmbientStrength() const {
        return ambientStrength;
    }

    void setAmbientStrength(float strength);

    float getRoughness() const {
        return roughness;
    }

    void setRoughness(float roughness);

    int32_t getNumLights() const {
        return numLights;
    }

    void setNumLights(int32_t numLights);

    int32_t getAttenuationIndex() const {
        return attenuationIndex;
    }

    void setAttenuationIndex(int32_t attenuationIndex);

    glm::vec4 getAttenuationValuesFromIndex(int32_t attenuationIndex) const;

   private:
    std::shared_ptr<scene::GameCamera> camera;
    std::unique_ptr<sponge::platform::opengl::scene::Cube> cube;

    bool metallic = false;
    float ambientStrength = .03F;
    float ao = .25F;
    float roughness = .5F;
    int32_t numLights = 6;
    int32_t attenuationIndex = 4;

    static bool onMouseButtonPressed(
        const sponge::event::MouseButtonPressedEvent& event);

    static bool onMouseButtonReleased(
        const sponge::event::MouseButtonReleasedEvent& event);

    bool onMouseScrolled(const sponge::event::MouseScrolledEvent& event) const;

    bool onWindowResize(const sponge::event::WindowResizeEvent& event) const;
};

}  // namespace game::layer
