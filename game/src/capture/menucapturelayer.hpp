#pragma once

#include "layer/layer.hpp"

#include <string>

namespace game::capture {

// Debug capture of a menu screen: lets the screen settle, writes the finished
// back buffer as capture_menu_<screen>.png and exits. Pushed after every other
// layer so it reads a complete frame. Render thread only.
class MenuCaptureLayer final : public sponge::layer::Layer {
public:
    explicit MenuCaptureLayer(std::string screen);

    bool onUpdate(double elapsedTime) override;

private:
    std::string screen;
    double      elapsed = 0.;
    bool        written = false;
};

}  // namespace game::capture
