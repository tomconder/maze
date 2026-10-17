#include "ui/slider.hpp"

#include <fmt/format.h>
#include <yoga/Yoga.h>

#include "ui/rowlayout.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace game::ui {

namespace {
constexpr float            disabledBrightness = 0.4F;
constexpr std::string_view leftArrowText      = "< ";
constexpr std::string_view rightArrowText     = " >";
}  // namespace

Slider::Slider(const SliderCreateInfo& createInfo) :
    font(createInfo.font),
    fontSize(createInfo.fontSize),
    textColor(createInfo.textColor),
    arrowDisabledColor(createInfo.arrowDisabledColor),
    handleRadius(createInfo.handleRadius),
    handleBorderWidth(createInfo.handleBorderWidth),
    trackColor(createInfo.trackColor),
    fillColor(createInfo.fillColor),
    handleColor(createInfo.handleColor),
    handleBorderColor(createInfo.handleBorderColor),
    quad(std::make_unique<sponge::platform::opengl::scene::Quad>()) {}

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

void Slider::setValueText(std::string text) {
    valueText = std::move(text);
}

void Slider::setDisabled(const bool isDisabled) {
    disabled = isDisabled;
}

void Slider::setFontSize(const uint32_t size) {
    if (fontSize == size) {
        return;
    }
    fontSize = size;
}

void Slider::attach(const YGNodeRef row) {
    rowNode = row;
}

void Slider::onUpdate(const float y, const std::string_view label) {
    const auto part = [&](const int index) {
        return partRect(rowNode, index, y);
    };
    const auto labelRect      = part(labelPart);
    const auto leftArrowRect  = part(sliderparts::leftArrow);
    const auto track          = part(sliderparts::track);
    const auto rightArrowRect = part(sliderparts::rightArrow);
    const auto valueRect      = part(sliderparts::value);

    const float trackW  = track.width;
    const float trackH  = track.height;
    const float radius  = handleRadius * scale();
    const float tStartX = track.x;
    const float trackY  = track.y;

    // Disabled: everything drawn with a fraction of its brightness.
    const float dim   = disabled ? disabledBrightness : 1.F;
    const auto  faded = [dim](glm::vec4 color) {
        return glm::vec4(glm::vec3(color) * dim, color.a);
    };

    quad->render({ tStartX, trackY }, { tStartX + trackW, trackY + trackH },
                 faded(trackColor), trackH / 2.F);

    const float fillWidth = trackW * value;
    if (fillWidth > 0.F) {
        quad->render({ tStartX, trackY },
                     { tStartX + fillWidth, trackY + trackH }, faded(fillColor),
                     trackH / 2.F);
    }

    const float handleCenterX = tStartX + fillWidth;
    const float handleCenterY = trackY + trackH / 2.F;
    quad->render({ handleCenterX - radius, handleCenterY - radius },
                 { handleCenterX + radius, handleCenterY + radius },
                 faded(handleColor), radius, handleBorderWidth * scale(),
                 faded(handleBorderColor));

    const auto& color = disabled ? arrowDisabledColor : textColor;
    font->beginPass(fontSize);
    font->render(label, { labelRect.x, labelRect.y }, color);
    font->render(leftArrowText, { leftArrowRect.x, leftArrowRect.y },
                 !disabled && value > 0.F ? textColor : arrowDisabledColor);
    font->render(rightArrowText, { rightArrowRect.x, rightArrowRect.y },
                 !disabled && value < 1.F ? textColor : arrowDisabledColor);

    const auto valueStr =
        valueText.empty() ?
            fmt::format("{}%", static_cast<int>(std::lround(value * 100.F))) :
            valueText;
    font->render(valueStr, { valueRect.x, valueRect.y }, color, true);
    font->endPass();
}

bool Slider::isInsideTrack(const float mouseX) const {
    const auto  track  = nodeRect(YGNodeGetChild(rowNode, sliderparts::track));
    const float radius = handleRadius * scale();
    return mouseX >= track.x - radius &&
           mouseX <= track.x + track.width + radius;
}

float Slider::valueAtX(const float mouseX) const {
    const auto track = nodeRect(YGNodeGetChild(rowNode, sliderparts::track));
    return std::clamp((mouseX - track.x) / track.width, 0.F, 1.F);
}

int Slider::arrowAt(const float mouseX) const {
    const auto left = nodeRect(YGNodeGetChild(rowNode, sliderparts::leftArrow));
    const auto track = nodeRect(YGNodeGetChild(rowNode, sliderparts::track));
    const auto right =
        nodeRect(YGNodeGetChild(rowNode, sliderparts::rightArrow));
    const auto valueRect =
        nodeRect(YGNodeGetChild(rowNode, sliderparts::value));
    if (mouseX >= left.x && mouseX < track.x) {
        return -1;
    }
    if (mouseX >= right.x && mouseX < valueRect.x) {
        return 1;
    }
    return 0;
}

}  // namespace game::ui
