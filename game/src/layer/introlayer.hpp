#pragma once

#include "sponge.hpp"

namespace game::layer {

enum class MenuItem : int { NewGame = 0, Options, Quit, Count };

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
    bool     optionsFlag   = false;
    bool     startGameFlag = false;
    bool     quitFlag      = false;
    MenuItem selectedItem  = MenuItem::NewGame;

    void recalculateLayout(float width, float height) const;

    bool onKeyPressed(const sponge::event::KeyPressedEvent& event);

    bool onMouseButtonPressed(
        const sponge::event::MouseButtonPressedEvent& event);

    bool onMouseMoved(const sponge::event::MouseMovedEvent& event) const;

    bool onWindowResize(const sponge::event::WindowResizeEvent& event) const;
};
}  // namespace game::layer
