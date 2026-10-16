#include "ui/slider.hpp"

#include <fmt/format.h>
#include <yoga/Yoga.h>

#include "ui/rowparts.hpp"

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

// The children of the row, left to right.
constexpr int labelPart      = 0;
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
    font->render(label, { labelRect.x, labelRect.y }, textColor);
    font->render(leftArrowText, { leftArrowRect.x, leftArrowRect.y },
                 value > 0.F ? textColor : arrowDisabledColor);
    font->render(rightArrowText, { rightArrowRect.x, rightArrowRect.y },
                 value < 1.F ? textColor : arrowDisabledColor);

    const auto valueStr =
        fmt::format("{}%", static_cast<int>(std::lround(value * 100.F)));
    font->render(valueStr, { valueRect.x, valueRect.y }, textColor, true);
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
    const auto left  = nodeRect(YGNodeGetChild(rowNode, leftArrowPart));
    const auto track = nodeRect(YGNodeGetChild(rowNode, trackPart));
    const auto right = nodeRect(YGNodeGetChild(rowNode, rightArrowPart));
    const auto value = nodeRect(YGNodeGetChild(rowNode, valuePart));
    if (mouseX >= left.x && mouseX < track.x) {
        return -1;
    }
    if (mouseX >= right.x && mouseX < value.x) {
        return 1;
    }
    return 0;
}

}  // namespace game::ui
