#pragma once

#include "gamecamera.h"
#include "sponge.h"

class MazeLayer final : public sponge::layer::Layer {
   public:
    MazeLayer();
    void onAttach() override;
    void onDetach() override;
    void onEvent(sponge::event::Event& event) override;
    bool onUpdate(double elapsedTime) override;

    std::shared_ptr<GameCamera> getCamera() const {
        return camera;
    }

   private:
    std::shared_ptr<GameCamera> camera;

    static constexpr float keyboardSpeed = .1F;
    static constexpr float mouseSpeed = .1F;

    bool onKeyPressed(const sponge::event::KeyPressedEvent& event);
    bool onMouseButtonPressed(
        const sponge::event::MouseButtonPressedEvent& event);
    bool onMouseButtonReleased(
        const sponge::event::MouseButtonReleasedEvent& event);
    bool onMouseMoved(const sponge::event::MouseMovedEvent& event);
    bool onMouseScrolled(const sponge::event::MouseScrolledEvent& event);
    bool onWindowResize(const sponge::event::WindowResizeEvent& event);
};
