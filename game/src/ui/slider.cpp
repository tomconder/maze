#include "ui/slider.hpp"

#include <fmt/format.h>
#include <yoga/Yoga.h>

#include "ui/rowparts.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace game::ui {

namespace {
// The sizes in SliderCreateInfo are for this font size.
constexpr float            referenceFontSize = 32.F;
constexpr float            baseValueGap      = 12.F;
constexpr float            disabledAlpha     = 0.4F;
constexpr std::string_view fullValueText     = "100%";
constexpr std::string_view leftArrowText     = "< ";
constexpr std::string_view rightArrowText    = " >";

// The children of the row after the label, left to right.
constexpr int leftArrowPart  = 1;
constexpr int trackPart      = 2;
constexpr int rightArrowPart = 3;
constexpr int valuePart      = 4;
constexpr int partCount      = 5;
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
    labelIndent(createInfo.labelIndent),
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
    recomputeCachedLengths();
}

void Slider::recomputeCachedLengths() {
    scale = static_cast<float>(fontSize) / referenceFontSize;
    percentLen =
        static_cast<float>(font->getLength(fullValueText, fontSize, true));
    leftArrowLen = static_cast<float>(font->getLength(leftArrowText, fontSize));
    rightArrowLen =
        static_cast<float>(font->getLength(rightArrowText, fontSize));
    sizeNodes();
}

void Slider::attach(const YGNodeRef row) {
    rowNode = row;
    attachRowParts(row, partCount, textMarginLeft);
    sizeNodes();
}

void Slider::sizeNodes() const {
    if (rowNode == nullptr) {
        return;
    }
    YGNodeStyleSetGap(rowNode, YGGutterColumn, baseValueGap * scale);

    const auto textHeight = static_cast<float>(font->getHeight(fontSize));
    setPartSize(rowNode, labelPart, YGUndefined, textHeight);
    YGNodeStyleSetMargin(YGNodeGetChild(rowNode, labelPart), YGEdgeLeft,
                         labelIndent * scale);
    setPartSize(rowNode, leftArrowPart, leftArrowLen, textHeight);
    setPartSize(rowNode, trackPart, trackWidth * scale, trackHeight * scale);
    setPartSize(rowNode, rightArrowPart, rightArrowLen, textHeight);
    setPartSize(rowNode, valuePart, percentLen, textHeight);
}

void Slider::onUpdate(const float y, const std::string_view label) {
    const auto part = [&](const int index) {
        return partRect(rowNode, index, y);
    };
    const auto labelRect      = part(labelPart);
    const auto leftArrowRect  = part(leftArrowPart);
    const auto track          = part(trackPart);
    const auto rightArrowRect = part(rightArrowPart);
    const auto valueRect      = part(valuePart);

    const float trackW  = track.width;
    const float trackH  = track.height;
    const float radius  = handleRadius * scale;
    const float tStartX = track.x;
    const float trackY  = track.y;

    // Disabled: everything drawn with a fraction of its alpha.
    const float dim   = disabled ? disabledAlpha : 1.F;
    const auto  faded = [dim](glm::vec4 color) {
        color.a *= dim;
        return color;
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
                 faded(handleColor), radius, handleBorderWidth * scale,
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
    const auto  track  = nodeRect(YGNodeGetChild(rowNode, trackPart));
    const float radius = handleRadius * scale;
    return mouseX >= track.x - radius &&
           mouseX <= track.x + track.width + radius;
}

float Slider::valueAtX(const float mouseX) const {
    const auto track = nodeRect(YGNodeGetChild(rowNode, trackPart));
    return std::clamp((mouseX - track.x) / track.width, 0.F, 1.F);
}

int Slider::arrowAt(const float mouseX) const {
    const auto left      = nodeRect(YGNodeGetChild(rowNode, leftArrowPart));
    const auto track     = nodeRect(YGNodeGetChild(rowNode, trackPart));
    const auto right     = nodeRect(YGNodeGetChild(rowNode, rightArrowPart));
    const auto valueRect = nodeRect(YGNodeGetChild(rowNode, valuePart));
    if (mouseX >= left.x && mouseX < track.x) {
        return -1;
    }
    if (mouseX >= right.x && mouseX < valueRect.x) {
        return 1;
    }
    return 0;
}

}  // namespace game::ui
