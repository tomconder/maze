#pragma once

#include "sponge.hpp"

namespace game::layer {

enum class IntroMenuItem : uint8_t { NewGame = 0, Options, Quit, Count };

class IntroLayer final : public sponge::layer::Layer {
public:
    IntroLayer();

    void onAttach() override;

    void onDetach() override;

    void onEvent(sponge::event::Event& event) override;

    bool onUpdate(double elapsedTime) override;

    void beginFadeIn(double duration);

private:
    IntroMenuItem selectedItem       = IntroMenuItem::NewGame;
    bool          wasActiveLastFrame = false;
    bool          isFadingIn         = false;
    double        fadeInDuration     = 0.0;
    double        fadeInAccumulator  = 0.0;

    static void recalculateLayout(float width, float height);

    bool onMouseButtonPressed(
        const sponge::event::MouseButtonPressedEvent& event);

    bool onMouseMoved(const sponge::event::MouseMovedEvent& event) const;

    bool onWindowResize(const sponge::event::WindowResizeEvent& event) const;

    static void clearHoveredItems();
};
}  // namespace game::layer
