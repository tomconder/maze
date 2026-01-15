#pragma once

#include "sponge.hpp"

#include <memory>
#include <string>

namespace game::ui {

enum class ButtonAlignType : uint8_t { CenterAligned = 0, LeftAligned };

struct ButtonCreateInfo {
    glm::vec2       topLeft;
    glm::vec2       bottomRight;
    std::string     message;
    uint32_t        fontSize;
    std::string     fontName;
    glm::vec4       buttonColor;
    glm::vec3       textColor;
    uint32_t        marginLeft   = 0;
    float           cornerRadius = 0.F;
    float           borderWidth  = 0.F;
    glm::vec4       borderColor  = glm::vec4(0.F);
    ButtonAlignType alignType    = ButtonAlignType::CenterAligned;
};

class Button {
public:
    explicit Button(const ButtonCreateInfo& createInfo);

    bool onUpdate(double elapsedTime) const;

    bool isInside(const glm::vec2& position) const;

    void setButtonColor(const glm::vec4& val);

    void setTextColor(const glm::vec3& val);

    void setBorderWidth(float val);

    void setBorderColor(const glm::vec4& val);

    void setPosition(const glm::vec2& topLeft, const glm::vec2& bottomRight);

    void setAlignType(const ButtonAlignType& val);

    void setHover(const bool value) {
        hover = value;
    }

    bool hasHover() const {
        return hover;
    }

    ButtonAlignType getAlignType() const {
        return alignType;
    }

private:
    glm::vec2       top;
    glm::vec2       bottom;
    std::string     text;
    uint32_t        textSize;
    std::string     textFontName;
    glm::vec4       color;
    glm::vec3       textColor;
    uint32_t        length;
    uint32_t        marginLeft;
    float           cornerRadius;
    float           borderWidth;
    glm::vec4       borderColor;
    ButtonAlignType alignType;

    glm::vec2 textPosition;
    bool      hover = false;

    std::shared_ptr<sponge::platform::opengl::scene::MSDFFont> font;
    std::unique_ptr<sponge::platform::opengl::scene::Quad>     quad;
};
}  // namespace game::ui
