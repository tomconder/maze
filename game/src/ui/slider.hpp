#pragma once

#include "platform/opengl/scene/bitmapfont.hpp"
#include "platform/opengl/scene/quad.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
#include <string_view>
#include <tuple>

namespace game::ui {

struct SliderCreateInfo {
    std::shared_ptr<sponge::platform::opengl::scene::BitmapFont> font;
    uint32_t                                                     fontSize;
    glm::vec3                                                    textColor;
    glm::vec3 arrowDisabledColor;
    float     textMarginLeft;
    float     trackWidth        = 200.F;
    float     trackHeight       = 6.F;
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
class Slider {
public:
    explicit Slider(const SliderCreateInfo& createInfo);

    float getValue() const;
    void  setValue(float newValue);

    // Nudges the value by delta (positive or negative), clamped to [0, 1].
    // Returns true if the value actually changed, for keyboard/gamepad steps.
    bool step(float delta);

    void setFontSize(uint32_t size);

    void onUpdate(float x, float y, float w, float h, std::string_view label);

    // Track hit zone, widened by the handle radius on both ends so grabbing
    // the handle at either extreme works. y/h aren't needed: callers already
    // row-bound the click before asking, same contract as SelectList.
    bool isInsideTrack(float mouseX, float x, float w) const;

    // Maps a mouse x position to a value in [0, 1], for click-to-jump and
    // drag updates.
    float valueAtX(float mouseX, float x, float w) const;

private:
    std::shared_ptr<sponge::platform::opengl::scene::BitmapFont> font;
    uint32_t                                                     fontSize;
    glm::vec3                                                    textColor;
    glm::vec3 arrowDisabledColor;
    float     textMarginLeft;
    float     trackWidth;
    float     trackHeight;
    float     handleRadius;
    float     handleBorderWidth;
    glm::vec4 trackColor;
    glm::vec4 fillColor;
    glm::vec4 handleColor;
    glm::vec4 handleBorderColor;

    float value         = 1.F;
    float percentLen    = 0.F;  // cached width of "100%" at fontSize
    float leftArrowLen  = 0.F;  // cached width of "< " at fontSize
    float rightArrowLen = 0.F;  // cached width of " >" at fontSize

    std::unique_ptr<sponge::platform::opengl::scene::Quad> quad;

    void recomputeCachedLengths();

    // Left edge of the arrow/track/value block: {leftArrowX, trackStartX,
    // rightArrowX, valueX}.
    std::tuple<float, float, float, float> computeLayout(float x,
                                                         float w) const;
};

}  // namespace game::ui
