#pragma once

#include "gamecamera.h"
#include "sponge.h"

class MazeLayer : public Sponge::Layer {
   public:
    void onAttach() override;
    void onDetach() override;
    void onEvent(Sponge::Event& event) override;
    bool onUpdate(uint32_t elapsedTime) override;

   private:
    std::unique_ptr<GameCamera> camera;

    bool onMouseMoved(const Sponge::MouseMovedEvent& event);
    bool onMouseScrolled(const Sponge::MouseScrolledEvent& event);
    bool onWindowResize(const Sponge::WindowResizeEvent& event);
};
