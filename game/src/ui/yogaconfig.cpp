#include "ui/yogaconfig.hpp"

#include <algorithm>
#include <cstdlib>
#include <ranges>
#include <string>
#include <unordered_map>

namespace game::ui {

namespace {
const std::unordered_map<std::string_view, YGFlexDirection> flexDirectionMap = {
    { "column", YGFlexDirectionColumn },
    { "row", YGFlexDirectionRow },
    { "column-reverse", YGFlexDirectionColumnReverse },
    { "row-reverse", YGFlexDirectionRowReverse },
};

const std::unordered_map<std::string_view, YGJustify> justifyMap = {
    { "flex-start", YGJustifyFlexStart },
    { "center", YGJustifyCenter },
    { "flex-end", YGJustifyFlexEnd },
    { "space-between", YGJustifySpaceBetween },
    { "space-around", YGJustifySpaceAround },
    { "space-evenly", YGJustifySpaceEvenly },
};

const std::unordered_map<std::string_view, YGAlign> alignMap = {
    { "auto", YGAlignAuto },
    { "flex-start", YGAlignFlexStart },
    { "center", YGAlignCenter },
    { "flex-end", YGAlignFlexEnd },
    { "stretch", YGAlignStretch },
    { "baseline", YGAlignBaseline },
    { "space-between", YGAlignSpaceBetween },
    { "space-around", YGAlignSpaceAround },
};

const std::unordered_map<std::string_view, YGEdge> edgeMap = {
    { "margin", YGEdgeAll },          { "marginTop", YGEdgeTop },
    { "marginBottom", YGEdgeBottom }, { "marginLeft", YGEdgeLeft },
    { "marginRight", YGEdgeRight },   { "padding", YGEdgeAll },
    { "paddingTop", YGEdgeTop },      { "paddingBottom", YGEdgeBottom },
    { "paddingLeft", YGEdgeLeft },    { "paddingRight", YGEdgeRight },
};

float parseFloat(std::string_view str) {
    const std::string temp(str);
    const char*       start = temp.c_str();
    char*             end   = nullptr;
    return std::strtof(start, &end);
}

std::string_view trim(std::string_view str) {
    const auto isNotSpace = [](unsigned char ch) { return !std::isspace(ch); };

    const auto start = std::ranges::find_if(str, isNotSpace);
    const auto end =
        std::ranges::find_if(str | std::views::reverse, isNotSpace).base();

    return start < end ? std::string_view(&*start, std::distance(start, end)) :
                         std::string_view{};
}

bool endsWithPercent(std::string_view str) {
    return !str.empty() && str.back() == '%';
}

template <typename EnumType, typename SetterFunc>
void applyEnumProperty(
    YGNodeRef node, std::string_view value,
    const std::unordered_map<std::string_view, EnumType>& map,
    SetterFunc                                            setter) {
    if (const auto enumValue = map.find(value); enumValue != map.end()) {
        setter(node, enumValue->second);
    }
}

void applyDimension(YGNodeRef node, std::string_view value,
                    void (*setAbsolute)(YGNodeRef, float),
                    void (*setPercent)(YGNodeRef, float)) {
    if (endsWithPercent(value)) {
        const float percent = parseFloat(value.substr(0, value.size() - 1));
        setPercent(node, percent);
    } else {
        setAbsolute(node, parseFloat(value));
    }
}
}  // namespace

void YogaConfig::applyConfig(YGNodeRef node, const std::string_view config) {
    applyStyle(node, config);
}

void YogaConfig::applyStyle(YGNodeRef node, const std::string_view style) {
    auto properties =
        style | std::views::split(';') | std::views::transform([](auto&& prop) {
            return std::string_view(prop.begin(), prop.end());
        });

    for (const auto& property : properties) {
        if (property.empty()) {
            continue;
        }

        const auto colonPos = property.find(':');
        if (colonPos == std::string_view::npos) {
            continue;
        }

        const auto key   = trim(property.substr(0, colonPos));
        const auto value = trim(property.substr(colonPos + 1));

        if (!key.empty() && !value.empty()) {
            applyProperty(node, key, value);
        }
    }
}

void YogaConfig::applyProperty(YGNodeRef node, const std::string_view key,
                               const std::string_view value) {
    if (key == "flexDirection") {
        applyEnumProperty(node, value, flexDirectionMap,
                          YGNodeStyleSetFlexDirection);
    } else if (key == "justifyContent") {
        applyEnumProperty(node, value, justifyMap,
                          YGNodeStyleSetJustifyContent);
    } else if (key == "alignItems") {
        applyEnumProperty(node, value, alignMap, YGNodeStyleSetAlignItems);
    } else if (key == "alignSelf") {
        applyEnumProperty(node, value, alignMap, YGNodeStyleSetAlignSelf);
    } else if (key == "width") {
        applyDimension(node, value, YGNodeStyleSetWidth,
                       YGNodeStyleSetWidthPercent);
    } else if (key == "height") {
        applyDimension(node, value, YGNodeStyleSetHeight,
                       YGNodeStyleSetHeightPercent);
    } else if (key == "flex") {
        YGNodeStyleSetFlex(node, parseFloat(value));
    } else if (key.starts_with("margin")) {
        if (const auto edge = edgeMap.find(key); edge != edgeMap.end()) {
            YGNodeStyleSetMargin(node, edge->second, parseFloat(value));
        }
    } else if (key.starts_with("padding")) {
        if (const auto edge = edgeMap.find(key); edge != edgeMap.end()) {
            YGNodeStyleSetPadding(node, edge->second, parseFloat(value));
        }
    }
}

}  // namespace game::ui
