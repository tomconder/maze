#pragma once

#include "scene/orthocamera.hpp"
#include "ui/button.hpp"

#include <yoga/Yoga.h>
#include <memory>

namespace game::layer {
class IntroLayer final : public sponge::layer::Layer {
public:
    IntroLayer();

    void onAttach() override;

    void onDetach() override;

    void onEvent(sponge::event::Event& event) override;

    bool onUpdate(double elapsedTime) override;

    [[nodiscard]] bool shouldStartGame() const {
        return startGameFlag;
    }

    [[nodiscard]] bool shouldQuit() const {
        return quitFlag;
    }

private:
    std::shared_ptr<scene::OrthoCamera> orthoCamera;

    std::unique_ptr<sponge::platform::opengl::scene::Quad> backgroundQuad;
    std::unique_ptr<ui::Button>                            newGameButton;
    std::unique_ptr<ui::Button>                            optionsButton;
    std::unique_ptr<ui::Button>                            quitButton;

    YGNodeRef rootNode    = nullptr;
    YGNodeRef titleNode   = nullptr;
    YGNodeRef newGameNode = nullptr;
    YGNodeRef optionsNode = nullptr;
    YGNodeRef quitNode    = nullptr;

    bool optionsFlag   = false;
    bool startGameFlag = false;
    bool quitFlag      = false;
    int  selectedIndex = 0;
    std::unique_ptr<sponge::platform::opengl::scene::Quad> quad;

    void recalculateLayout(float width, float height) const;

    bool onKeyPressed(const sponge::event::KeyPressedEvent& event);

    bool onMouseButtonPressed(
        const sponge::event::MouseButtonPressedEvent& event);

    bool onMouseMoved(const sponge::event::MouseMovedEvent& event);

    bool onWindowResize(const sponge::event::WindowResizeEvent& event) const;
};
}  // namespace game::layer
