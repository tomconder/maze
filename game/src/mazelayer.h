#pragma once

#include "gamecamera.h"
#include "sponge.h"

class MazeLayer : public sponge::Layer {
   public:
    MazeLayer();
    void onAttach() override;
    void onDetach() override;
    void onEvent(sponge::Event& event) override;
    bool onUpdate(uint32_t elapsedTime) override;

   private:
    std::unique_ptr<GameCamera> camera;

    static constexpr float keyboardSpeed = .1F;
    static constexpr float mouseSpeed = .1F;

    bool onKeyPressed(const sponge::KeyPressedEvent& event);
    bool onMouseMoved(const sponge::MouseMovedEvent& event);
    bool onMouseScrolled(const sponge::MouseScrolledEvent& event);
    bool onWindowResize(const sponge::WindowResizeEvent& event);
};
