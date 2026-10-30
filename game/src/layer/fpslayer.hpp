#pragma once

#include "layer/layer.hpp"
#include "platform/opengl/scene/bitmapfont.hpp"
#include "platform/opengl/scene/quad.hpp"
#include "scene/orthocamera.hpp"

#include <array>
#include <cstddef>
#include <memory>

namespace game::layer {

// A frames-per-second counter in the upper right corner, in every build. F3
// shows or hides it (GameAction::ToggleFps) and the choice is saved as
// video.showFps. It stays active so it can read the key; `visible` gates the
// drawing. The figure is the mean of the last 60 frame times, as in ImGui's
// io.Framerate.
class FpsLayer final : public sponge::layer::Layer {
public:
    FpsLayer();

    void onAttach() override;

    bool onUpdate(double elapsedTime) override;

private:
    static constexpr size_t windowFrames = 60;

    std::array<float, windowFrames> deltas{};
    size_t                          next    = 0;
    size_t                          count   = 0;
    double                          sum     = 0.;
    bool                            visible = false;

    std::shared_ptr<sponge::platform::opengl::scene::BitmapFont> font;
    std::shared_ptr<game::scene::OrthoCamera>                    orthoCamera;
    std::unique_ptr<sponge::platform::opengl::scene::Quad>       quad;

    void sample(double elapsedTime);

    void draw() const;
};

}  // namespace game::layer
