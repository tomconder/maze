#pragma once

#include "scene/orthocamera.hpp"
#include "sponge.hpp"

#include <memory>

namespace game::layer {
class SplashScreenLayer final : public sponge::layer::Layer {
public:
    SplashScreenLayer();

    void onAttach() override;

    void onDetach() override;

    void onEvent(sponge::event::Event& event) override;

    bool onUpdate(double elapsedTime) override;

    [[nodiscard]] bool shouldDismiss() const {
        return shouldDismissFlag;
    }

    [[nodiscard]] bool isFading() const {
        return isFadingFlag;
    }

private:
    std::shared_ptr<scene::OrthoCamera> orthoCamera;

    std::unique_ptr<sponge::platform::opengl::scene::Sprite> logoSprite;
    std::unique_ptr<sponge::platform::opengl::scene::Quad>   backgroundQuad;

    double elapsedTimeAccumulator = 0.0;
    double fadeTimeAccumulator    = 0.0;
    float  currentAlpha           = 1.0F;
    bool   shouldDismissFlag      = false;
    bool   isFadingFlag           = false;

    [[nodiscard]] glm::vec2 calculateLogoPosition() const;

    bool onKeyPressed(const sponge::event::KeyPressedEvent& event);

    bool onWindowResize(const sponge::event::WindowResizeEvent& event) const;
};
}  // namespace game::layer
