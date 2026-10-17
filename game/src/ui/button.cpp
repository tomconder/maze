#include "ui/button.hpp"

#include "core/file.hpp"
#include "platform/audio/audio.hpp"
#include "ui/rowparts.hpp"
#include "ui/yogarect.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace game::ui {

Button::Button(const ButtonCreateInfo& createInfo) :
    text(createInfo.message),
    textSize(createInfo.fontSize),
    color(createInfo.buttonColor),
    textColor(createInfo.textColor),
    marginLeft(createInfo.marginLeft),
    cornerRadius(createInfo.cornerRadius),
    borderWidth(createInfo.borderWidth),
    borderColor(createInfo.borderColor),
    alignType(createInfo.alignType) {
    font   = createInfo.font;
    length = font->getLength(text, textSize);

    quad = std::make_unique<sponge::platform::opengl::scene::Quad>();
}

void Button::attach(const YGNodeRef box) {
    node = box;
    YGNodeStyleSetFlexDirection(node, YGFlexDirectionRow);
    YGNodeStyleSetAlignItems(node, YGAlignCenter);
    YGNodeStyleSetPadding(node, YGEdgeHorizontal,
                          static_cast<float>(marginLeft));
    YGNodeInsertChild(node, YGNodeNew(), 0);
    sizeNodes();
}

void Button::sizeNodes() const {
    if (node == nullptr) {
        return;
    }
    YGNodeStyleSetJustifyContent(
        node, alignType == ButtonAlignType::CenterAligned ? YGJustifyCenter :
                                                            YGJustifyFlexStart);
    // The text is centred on the font size, not the line height.
    setPartSize(node, 0, static_cast<float>(length),
                static_cast<float>(textSize));
}

bool Button::onUpdate(const double) const {
    const auto box      = nodeRect(node);
    const auto textRect = nodeRect(YGNodeGetChild(node, 0));
    quad->render({ box.x, box.y }, { box.x + box.width, box.y + box.height },
                 color, cornerRadius, borderWidth, borderColor);
    font->beginPass(textSize);
    font->render(text, { textRect.x, textRect.y }, textColor);
    font->endPass();

    return true;
}

bool Button::isInside(const glm::vec2& position) const {
    const auto box = nodeRect(node);
    return box.x <= position.x && box.y <= position.y &&
           box.x + box.width >= position.x && box.y + box.height >= position.y;
}

void Button::setButtonColor(const glm::vec4& val) {
    color = glm::vec4(val);
}

void Button::setTextColor(const glm::vec3& val) {
    textColor = glm::vec3(val);
}

void Button::setBorderWidth(const float val) {
    borderWidth = val;
}

void Button::setBorderColor(const glm::vec4& val) {
    borderColor = val;
}

void Button::setAlignType(const ButtonAlignType& val) {
    alignType = val;
    sizeNodes();
}

void Button::setMessage(std::string_view message) {
    if (text == message) {
        return;
    }

    text   = message;
    length = font->getLength(text, textSize);
    sizeNodes();
}

void Button::setFontSize(const uint32_t size) {
    if (textSize == size) {
        return;
    }
    textSize = size;
    length   = font->getLength(text, textSize);
    sizeNodes();
}

std::unique_ptr<Button> makeMenuButton(
    std::string_view message, const uint32_t fontSize,
    std::shared_ptr<sponge::platform::opengl::scene::BitmapFont> font,
    const glm::vec4& buttonColor, const glm::vec3& textColor) {
    return std::make_unique<Button>(ButtonCreateInfo{
        .message      = std::string(message),
        .fontSize     = fontSize,
        .font         = std::move(font),
        .buttonColor  = buttonColor,
        .textColor    = textColor,
        .marginLeft   = 26,
        .cornerRadius = 12.F,
        .alignType    = ButtonAlignType::LeftAligned,
    });
}

void updateMenuButtonVisuals(Button* button, const bool selected,
                             const glm::vec4& selectedColor) {
    constexpr glm::vec4 hoverColor = { 0.84F, 0.84F, 0.84F, 0.14F };

    if (selected) {
        button->setBorderWidth(3.F);
        button->setBorderColor(glm::vec4{ 1.F });
        button->setButtonColor(selectedColor);
    } else if (!button->hasHover()) {
        button->setBorderWidth(0.F);
        button->setButtonColor(glm::vec4{ 0.F });
    } else {
        button->setBorderWidth(0.F);
        button->setButtonColor(hoverColor);
    }
}

void playHoverClick() {
    sponge::platform::audio::Audio::play(sponge::core::File::getResourceDir() +
                                         "/audio/click.wav");
}

void updateButtonHover(Button* button, const glm::vec2& pos) {
    if (!button->hasHover() && button->isInside(pos)) {
        button->setHover(true);
        playHoverClick();
    } else if (button->hasHover() && !button->isInside(pos)) {
        button->setHover(false);
    }
}
}  // namespace game::ui
