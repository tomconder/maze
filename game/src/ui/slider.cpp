#include "ui/slider.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string_view>

namespace game::ui {

namespace {
// The sizes in SliderCreateInfo are for this font size.
constexpr float            referenceFontSize = 32.F;
constexpr float            baseValueGap      = 12.F;
constexpr std::string_view fullValueText     = "100%";
constexpr std::string_view leftArrowText     = "< ";
constexpr std::string_view rightArrowText    = " >";
}  // namespace

Slider::Slider(const SliderCreateInfo& createInfo) :
    font(createInfo.font),
    fontSize(createInfo.fontSize),
    textColor(createInfo.textColor),
    arrowDisabledColor(createInfo.arrowDisabledColor),
    textMarginLeft(createInfo.textMarginLeft),
    trackWidth(createInfo.trackWidth),
    trackHeight(createInfo.trackHeight),
    handleRadius(createInfo.handleRadius),
    handleBorderWidth(createInfo.handleBorderWidth),
    trackColor(createInfo.trackColor),
    fillColor(createInfo.fillColor),
    handleColor(createInfo.handleColor),
    handleBorderColor(createInfo.handleBorderColor),
    quad(std::make_unique<sponge::platform::opengl::scene::Quad>()) {
    recomputeCachedLengths();
}

float Slider::getValue() const {
    return value;
}

void Slider::setValue(const float newValue) {
    value = std::clamp(newValue, 0.F, 1.F);
}

bool Slider::step(const float delta) {
    const float newValue = std::clamp(value + delta, 0.F, 1.F);
    const bool  changed  = newValue != value;
    value                = newValue;
    return changed;
}

void Slider::setFontSize(const uint32_t size) {
    if (fontSize == size) {
        return;
    }
    fontSize = size;
    recomputeCachedLengths();
}

void Slider::recomputeCachedLengths() {
    scale = static_cast<float>(fontSize) / referenceFontSize;
    percentLen =
        static_cast<float>(font->getLength(fullValueText, fontSize, true));
    leftArrowLen = static_cast<float>(font->getLength(leftArrowText, fontSize));
    rightArrowLen =
        static_cast<float>(font->getLength(rightArrowText, fontSize));
}

std::tuple<float, float, float, float>
    Slider::computeLayout(const float x, const float w) const {
    const float valueGap    = baseValueGap * scale;
    const float rightEdge   = x + w - textMarginLeft;
    const float valueX      = rightEdge - percentLen;
    const float rightArrowX = valueX - valueGap - rightArrowLen;
    const float trackEndX   = rightArrowX - valueGap;
    const float trackStartX = trackEndX - trackWidth * scale;
    const float leftArrowX  = trackStartX - valueGap - leftArrowLen;
    return { leftArrowX, trackStartX, rightArrowX, valueX };
}

void Slider::onUpdate(const float x, const float y, const float w,
                      const float h, const std::string_view label) {
    const auto [leftArrowX, tStartX, rightArrowX, valueX] = computeLayout(x, w);

    const float trackW = trackWidth * scale;
    const float trackH = trackHeight * scale;
    const float radius = handleRadius * scale;

    const float textY = std::floor(
        y + (h - static_cast<float>(font->getHeight(fontSize))) / 2.F);
    const float trackY = y + (h - trackH) / 2.F;

    quad->render({ tStartX, trackY }, { tStartX + trackW, trackY + trackH },
                 trackColor, trackH / 2.F);

    const float fillWidth = trackW * value;
    if (fillWidth > 0.F) {
        quad->render({ tStartX, trackY },
                     { tStartX + fillWidth, trackY + trackH }, fillColor,
                     trackH / 2.F);
    }

    const float handleCenterX = tStartX + fillWidth;
    const float handleCenterY = trackY + trackH / 2.F;
    quad->render({ handleCenterX - radius, handleCenterY - radius },
                 { handleCenterX + radius, handleCenterY + radius },
                 handleColor, radius, handleBorderWidth * scale,
                 handleBorderColor);

    font->beginPass(fontSize);
    font->render(label, { x + textMarginLeft, textY }, textColor);
    font->render(leftArrowText, { leftArrowX, textY },
                 value > 0.F ? textColor : arrowDisabledColor);
    font->render(rightArrowText, { rightArrowX, textY },
                 value < 1.F ? textColor : arrowDisabledColor);

    const auto valueStr =
        fmt::format("{}%", static_cast<int>(std::lround(value * 100.F)));
    font->render(valueStr, { valueX, textY }, textColor, true);
    font->endPass();
}

bool Slider::isInsideTrack(const float mouseX, const float x,
                           const float w) const {
    const auto [leftArrowX, tStartX, rightArrowX, valueX] = computeLayout(x, w);
    const float radius = handleRadius * scale;
    return mouseX >= tStartX - radius &&
           mouseX <= tStartX + trackWidth * scale + radius;
}

float Slider::valueAtX(const float mouseX, const float x, const float w) const {
    const auto [leftArrowX, tStartX, rightArrowX, valueX] = computeLayout(x, w);
    return std::clamp((mouseX - tStartX) / (trackWidth * scale), 0.F, 1.F);
}

int Slider::arrowAt(const float mouseX, const float x, const float w) const {
    const auto [leftArrowX, tStartX, rightArrowX, valueX] = computeLayout(x, w);
    if (mouseX >= leftArrowX && mouseX < tStartX) {
        return -1;
    }
    if (mouseX >= rightArrowX && mouseX < valueX) {
        return 1;
    }
    return 0;
}

}  // namespace game::ui
