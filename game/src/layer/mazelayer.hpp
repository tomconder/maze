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

    bool isWireframeActive() const {
        return activeWireframe;
    }

    void setWireframeActive(bool active);

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

   private:
    std::shared_ptr<scene::GameCamera> camera;
    std::unique_ptr<sponge::platform::opengl::scene::LightCube> lightCube;

    bool activeWireframe = false;
    bool metallic = false;
    float ambientStrength = .03F;
    float ao = .25F;
    float roughness = .5F;

    bool onKeyPressed(const sponge::event::KeyPressedEvent& event) const;

    static bool onMouseButtonPressed(
        const sponge::event::MouseButtonPressedEvent& event);

    static bool onMouseButtonReleased(
        const sponge::event::MouseButtonReleasedEvent& event);

    bool onMouseMoved(const sponge::event::MouseMovedEvent& event) const;

    bool onMouseScrolled(const sponge::event::MouseScrolledEvent& event) const;

    bool onWindowResize(const sponge::event::WindowResizeEvent& event) const;
};

}  // namespace game::layer
