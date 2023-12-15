#pragma once

#include "gamecamera.h"
#include "sponge.h"

class MazeLayer : public sponge::graphics::Layer {
   public:
    MazeLayer();
    void onAttach() override;
    void onDetach() override;
    void onEvent(sponge::event::Event& event) override;
    bool onUpdate(uint32_t elapsedTime) override;

   private:
    std::unique_ptr<GameCamera> camera;

    static constexpr float keyboardSpeed = .1F;
    static constexpr float mouseSpeed = .1F;

    bool onKeyPressed(const sponge::event::KeyPressedEvent& event);
    bool onMouseMoved(const sponge::event::MouseMovedEvent& event);
    bool onMouseScrolled(const sponge::event::MouseScrolledEvent& event);
    bool onWindowResize(const sponge::event::WindowResizeEvent& event);
};
