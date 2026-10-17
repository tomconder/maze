#pragma once

#include "platform/opengl/scene/bitmapfont.hpp"
#include "platform/opengl/scene/quad.hpp"

#include <glm/glm.hpp>
#include <yoga/Yoga.h>

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace game::ui {

enum class ButtonAlignType : uint8_t { CenterAligned = 0, LeftAligned };

struct ButtonCreateInfo {
    std::string                                                  message;
    uint32_t                                                     fontSize;
    std::shared_ptr<sponge::platform::opengl::scene::BitmapFont> font;
    glm::vec4                                                    buttonColor;
    glm::vec3                                                    textColor;
    uint32_t                                                     marginLeft = 0;
    float           cornerRadius = 0.F;
    float           borderWidth  = 0.F;
    glm::vec4       borderColor  = glm::vec4(0.F);
    ButtonAlignType alignType    = ButtonAlignType::CenterAligned;
};

class Button {
public:
    explicit Button(const ButtonCreateInfo& createInfo);

    // The button is the node's box; the text is its only child, aligned by
    // the align type. The node must be laid out before the button draws.
    void attach(YGNodeRef box);

    bool onUpdate(double elapsedTime) const;

    bool isInside(const glm::vec2& position) const;

    void setButtonColor(const glm::vec4& val);

    void setTextColor(const glm::vec3& val);

    void setBorderWidth(float val);

    void setBorderColor(const glm::vec4& val);

    void setAlignType(const ButtonAlignType& val);

    void setMessage(std::string_view message);

    void setFontSize(uint32_t size);

    void setHover(const bool value) {
        hover = value;
    }

    bool hasHover() const {
        return hover;
    }

    [[nodiscard]] ButtonAlignType getAlignType() const {
        return alignType;
    }

private:
    YGNodeRef       node = nullptr;
    std::string     text;
    uint32_t        textSize;
    glm::vec4       color;
    glm::vec3       textColor;
    uint32_t        length;
    uint32_t        marginLeft;
    float           cornerRadius;
    float           borderWidth;
    glm::vec4       borderColor;
    ButtonAlignType alignType;

    void sizeNodes() const;

    bool hover = false;

    std::shared_ptr<sponge::platform::opengl::scene::BitmapFont> font;
    std::unique_ptr<sponge::platform::opengl::scene::Quad>       quad;
};

std::unique_ptr<Button> makeMenuButton(
    std::string_view message, uint32_t fontSize,
    std::shared_ptr<sponge::platform::opengl::scene::BitmapFont> font,
    const glm::vec4& buttonColor, const glm::vec3& textColor);

void updateMenuButtonVisuals(Button* button, bool selected,
                             const glm::vec4& selectedColor);

void playHoverClick();

void updateButtonHover(Button* button, const glm::vec2& pos);
}  // namespace game::ui
