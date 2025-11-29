#include "ui/yogaconfig.hpp"

#include <algorithm>
#include <charconv>
#include <functional>
#include <ranges>
#include <string>
#include <unordered_map>

namespace game::ui {

namespace {

float parseFloat(std::string_view str) {
    float value = 0.0F;
    std::from_chars(str.data(), str.data() + str.size(), value);
    return value;
}

std::string_view trim(std::string_view str) {
    const auto isNotSpace = [](unsigned char ch) { return !std::isspace(ch); };

    const auto start = std::ranges::find_if(str, isNotSpace);
    const auto end =
        std::ranges::find_if(str | std::views::reverse, isNotSpace).base();

    return start < end ? std::string_view(&*start, std::distance(start, end)) :
                         std::string_view{};
}

bool endsWith(std::string_view str, std::string_view suffix) {
    return str.size() >= suffix.size() &&
           str.substr(str.size() - suffix.size()) == suffix;
}

}  // namespace

void YogaConfig::applyConfig(YGNodeRef node, const std::string_view config) {
    applyStyle(node, config);
}

void YogaConfig::applyStyle(YGNodeRef node, const std::string_view style) {
    size_t pos = 0;
    while (pos < style.size()) {
        const size_t colonPos = style.find(':', pos);
        if (colonPos == std::string_view::npos) {
            break;
        }

        const size_t semicolonPos = style.find(';', colonPos);
        const size_t endPos       = (semicolonPos == std::string_view::npos) ?
                                        style.size() :
                                        semicolonPos;

        const auto key = trim(style.substr(pos, colonPos - pos));
        const auto value =
            trim(style.substr(colonPos + 1, endPos - colonPos - 1));

        applyProperty(node, key, value);

        pos = (semicolonPos == std::string_view::npos) ? style.size() :
                                                         semicolonPos + 1;
    }
}

void YogaConfig::applyProperty(YGNodeRef node, const std::string_view key,
                               const std::string_view value) {
    using PropertyHandler = std::function<void(YGNodeRef, std::string_view)>;

    static const std::unordered_map<std::string_view, YGFlexDirection>
        flexDirectionMap = {
            { "column", YGFlexDirectionColumn },
            { "row", YGFlexDirectionRow },
            { "column-reverse", YGFlexDirectionColumnReverse },
            { "row-reverse", YGFlexDirectionRowReverse },
        };

    static const std::unordered_map<std::string_view, YGJustify> justifyMap = {
        { "flex-start", YGJustifyFlexStart },
        { "center", YGJustifyCenter },
        { "flex-end", YGJustifyFlexEnd },
        { "space-between", YGJustifySpaceBetween },
        { "space-around", YGJustifySpaceAround },
        { "space-evenly", YGJustifySpaceEvenly },
    };

    static const std::unordered_map<std::string_view, YGAlign> alignMap = {
        { "auto", YGAlignAuto },
        { "flex-start", YGAlignFlexStart },
        { "center", YGAlignCenter },
        { "flex-end", YGAlignFlexEnd },
        { "stretch", YGAlignStretch },
        { "baseline", YGAlignBaseline },
        { "space-between", YGAlignSpaceBetween },
        { "space-around", YGAlignSpaceAround },
    };

    static const std::unordered_map<std::string_view, YGEdge> edgeMap = {
        { "margin", YGEdgeAll },          { "marginTop", YGEdgeTop },
        { "marginBottom", YGEdgeBottom }, { "marginLeft", YGEdgeLeft },
        { "marginRight", YGEdgeRight },   { "padding", YGEdgeAll },
        { "paddingTop", YGEdgeTop },      { "paddingBottom", YGEdgeBottom },
        { "paddingLeft", YGEdgeLeft },    { "paddingRight", YGEdgeRight },
    };

    static const std::unordered_map<std::string_view, PropertyHandler>
        propertyHandlers = {
            { "flexDirection",
              [](YGNodeRef n, std::string_view v) {
                  if (const auto dir = flexDirectionMap.find(v);
                      dir != flexDirectionMap.end()) {
                      YGNodeStyleSetFlexDirection(n, dir->second);
                  }
              } },
            { "justifyContent",
              [](YGNodeRef n, std::string_view v) {
                  if (const auto justify = justifyMap.find(v);
                      justify != justifyMap.end()) {
                      YGNodeStyleSetJustifyContent(n, justify->second);
                  }
              } },
            { "alignItems",
              [](YGNodeRef n, std::string_view v) {
                  if (const auto align = alignMap.find(v);
                      align != alignMap.end()) {
                      YGNodeStyleSetAlignItems(n, align->second);
                  }
              } },
            { "alignSelf",
              [](YGNodeRef n, std::string_view v) {
                  if (const auto align = alignMap.find(v);
                      align != alignMap.end()) {
                      YGNodeStyleSetAlignSelf(n, align->second);
                  }
              } },
            { "width",
              [](YGNodeRef n, std::string_view v) {
                  if (endsWith(v, "%")) {
                      const float percent =
                          parseFloat(v.substr(0, v.size() - 1));
                      YGNodeStyleSetWidthPercent(n, percent);
                  } else {
                      YGNodeStyleSetWidth(n, parseFloat(v));
                  }
              } },
            { "height",
              [](YGNodeRef n, std::string_view v) {
                  if (endsWith(v, "%")) {
                      const float percent =
                          parseFloat(v.substr(0, v.size() - 1));
                      YGNodeStyleSetHeightPercent(n, percent);
                  } else {
                      YGNodeStyleSetHeight(n, parseFloat(v));
                  }
              } },
            { "flex",
              [](YGNodeRef n, std::string_view v) {
                  YGNodeStyleSetFlex(n, parseFloat(v));
              } },
        };

    if (const auto handler = propertyHandlers.find(key);
        handler != propertyHandlers.end()) {
        handler->second(node, value);
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
