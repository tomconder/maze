#pragma once

#include "platform/opengl/scene/bitmapfont.hpp"

#include <yoga/Yoga.h>

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace game::ui {

struct SelectListCreateInfo {
    std::shared_ptr<sponge::platform::opengl::scene::BitmapFont> font;
    uint32_t                                                     fontSize;
    glm::vec3                                                    textColor;
    glm::vec3 arrowDisabledColor;
    float     textMarginLeft;
    float     maxValueWidth = 0.F;
    // fixed-width digits for the selected value; see font::shape()
    bool tabularFigures = false;
};

// A value picked from a list, drawn as "label < value >". The parts are Yoga
// nodes in the row the list is attached to: the label takes the free width,
// then the left arrow, the value (a fixed width, so the arrows stay put) and
// the right arrow. The row must be laid out before the list draws or
// hit-tests.
class SelectList {
public:
    explicit SelectList(const SelectListCreateInfo& createInfo);

    // Adds the parts to the row node, which lives in the layer's layout.
    void attach(YGNodeRef row);

    void setItems(std::vector<std::string> newItems);
    void setMaxValueWidth(float width);
    void setFontSize(uint32_t size);

    size_t getSelectedIndex() const;
    void   setSelectedIndex(size_t index);

    // A disabled list is drawn grey and ignores cycling.
    void setDisabled(bool value);

    bool selectPrev();
    bool selectNext();

    std::string_view getValue() const;
    bool             hasLeft() const;
    bool             hasRight() const;

    // y is the top of the row as drawn, which differs from the row node when
    // the rows scroll.
    void onUpdate(float y, std::string_view label);

    float getValueCenterX() const;

    bool isInsideLeft(float mouseX) const;
    bool isInsideRight(float mouseX) const;

private:
    std::shared_ptr<sponge::platform::opengl::scene::BitmapFont> font;
    uint32_t                                                     fontSize;
    glm::vec3                                                    textColor;
    glm::vec3 arrowDisabledColor;
    float     textMarginLeft;
    float     maxValueWidth;
    bool      tabularFigures;
    bool      disabled = false;

    std::vector<std::string> items;
    size_t                   selectedIndex = 0;

    float leftLen  = 0.F;
    float rightLen = 0.F;

    YGNodeRef rowNode = nullptr;

    // Gives the nodes the sizes of the text.
    void sizeNodes() const;
};

}  // namespace game::ui
