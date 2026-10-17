#pragma once

#include "platform/opengl/scene/bitmapfont.hpp"
#include "platform/opengl/scene/quad.hpp"
#include "ui/layouttypes.hpp"
#include "ui/rowlayout.hpp"

#include <yoga/Yoga.h>

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace game::ui {

// handleRadius and handleBorderWidth are pixels at the 32 px menu font; the
// slider scales them with its font size.
struct SliderCreateInfo {
    std::shared_ptr<sponge::platform::opengl::scene::BitmapFont> font;
    uint32_t                                                     fontSize;
    glm::vec3                                                    textColor;
    glm::vec3 arrowDisabledColor;
    float     handleRadius      = 15.F;
    float     handleBorderWidth = 2.F;
    glm::vec4 trackColor        = { 1.F, 1.F, 1.F, 0.15F };
    glm::vec4 fillColor         = { 1.F, 0.84F, 0.2F, 1.F };
    glm::vec4 handleColor       = { 1.F, 1.F, 1.F, 1.F };
    glm::vec4 handleBorderColor = { 0.F, 0.F, 0.F, 0.4F };
};

// A 0..1 value on a horizontal track, drawn as "label < ==track== > 100%",
// the arrows dimming at the ends. Renders and hit-tests only, same split as
// Checkbox and SelectList — mouse drag state, keyboard/gamepad stepping and
// settings persistence are the owning layer's job, driven through
// isInsideTrack/valueAtX/setValue/step.
//
// The parts are Yoga nodes in the row the slider is attached to, which
// layoutSlider() builds and sizes: the label takes the free width, then the
// left arrow, the track, the right arrow and the value. The row must be laid
// out before the slider draws or hit-tests.
class Slider {
public:
    explicit Slider(const SliderCreateInfo& createInfo);

    float getValue() const;
    void  setValue(float newValue);

    // Nudges the value by delta (positive or negative), clamped to [0, 1].
    // Returns true if the value actually changed, for keyboard/gamepad steps.
    bool step(float delta);

    // Text drawn in place of the percentage. Empty goes back to the percentage.
    void setValueText(std::string text);

    // A disabled slider is drawn dim. Whether it still takes input is the
    // layer's call, same as the rest of the interaction.
    void setDisabled(bool isDisabled);

    // The row node, which lives in the layer's layout.
    void attach(YGNodeRef row);

    void setFontSize(uint32_t size);

    // y is the top of the row as drawn, which differs from the row node when
    // the rows scroll.
    void onUpdate(float y, std::string_view label);

    // Track hit zone, widened by the handle radius on both ends so grabbing
    // the handle at either extreme works. Only x is asked: callers already
    // row-bound the click before asking, same contract as SelectList.
    bool isInsideTrack(float mouseX) const;

    // Maps a mouse x position to a value in [0, 1], for click-to-jump and
    // drag updates.
    float valueAtX(float mouseX) const;

    // -1 over the left arrow, +1 over the right one, 0 elsewhere. Ask after
    // isInsideTrack: the widened track zone wins where the two overlap.
    int arrowAt(float mouseX) const;

private:
    std::shared_ptr<sponge::platform::opengl::scene::BitmapFont> font;
    uint32_t                                                     fontSize;
    glm::vec3                                                    textColor;
    glm::vec3 arrowDisabledColor;
    float     handleRadius;  // pixels at the 32 px font; multiply by scale
    float     handleBorderWidth;
    float     scale = 1.F;  // fontSize / 32
    glm::vec4 trackColor;
    glm::vec4 fillColor;
    glm::vec4 handleColor;
    glm::vec4 handleBorderColor;

    float       value    = 1.F;
    bool        disabled = false;
    std::string valueText;

    std::unique_ptr<sponge::platform::opengl::scene::Quad> quad;

    YGNodeRef rowNode = nullptr;
};

}  // namespace game::ui
