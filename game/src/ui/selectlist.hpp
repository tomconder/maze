#pragma once

#include "sponge.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace game::ui {

struct SelectListCreateInfo {
    std::string fontName;
    uint32_t    fontSize;
    glm::vec3   textColor;
    glm::vec3   arrowDisabledColor;
    float       textMarginLeft;
    float       maxValueWidth = 0.F;
};

class SelectList {
public:
    explicit SelectList(const SelectListCreateInfo& createInfo);

    void setItems(std::vector<std::string> items);
    void setMaxValueWidth(float width);

    size_t getSelectedIndex() const;
    void   setSelectedIndex(size_t index);

    bool selectPrev();
    bool selectNext();

    std::string_view getValue() const;
    bool             hasLeft() const;
    bool             hasRight() const;

    void onUpdate(float x, float y, float w, float h, std::string_view label);

    bool isInsideLeft(float mouseX, float x, float w) const;
    bool isInsideRight(float mouseX, float x, float w) const;

private:
    std::shared_ptr<sponge::platform::opengl::scene::MSDFFont> font;
    uint32_t                                                   fontSize;
    glm::vec3                                                  textColor;
    glm::vec3 arrowDisabledColor;
    float     textMarginLeft;
    float     maxValueWidth;

    std::vector<std::string> items;
    size_t                   selectedIndex = 0;

    float leftLen  = 0.F;
    float rightLen = 0.F;

    float getStartX(float x, float w) const;
};

}  // namespace game::ui
