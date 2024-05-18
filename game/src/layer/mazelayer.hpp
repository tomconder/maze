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

    bool isWireframeActive() const {
        return activeWireframe;
    }

    void setWireframeActive(bool active);

   private:
    std::shared_ptr<scene::GameCamera> camera;

    static constexpr float keyboardSpeed = .1F;
    static constexpr float mouseSpeed = .1F;

    bool activeWireframe = false;

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
