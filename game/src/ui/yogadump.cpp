#include "ui/yogadump.hpp"

#include <format>
#include <string>

namespace {

void addValue(std::string& style, const char* name, const YGValue value) {
    switch (value.unit) {
        case YGUnitPoint:
            style += std::format("{}: {}, ", name, value.value);
            break;
        case YGUnitPercent:
            style += std::format("{}: \"{}%\", ", name, value.value);
            break;
        default:
            break;
    }
}

void addNumber(std::string& style, const char* name, const float value) {
    if (!YGFloatIsUndefined(value) && value != 0.F) {
        style += std::format("{}: {}, ", name, value);
    }
}

void writeNode(const YGNodeRef node, const int depth, std::string& out) {
    std::string style;
    addValue(style, "width", YGNodeStyleGetWidth(node));
    addValue(style, "height", YGNodeStyleGetHeight(node));
    addValue(style, "maxHeight", YGNodeStyleGetMaxHeight(node));
    addValue(style, "margin", YGNodeStyleGetMargin(node, YGEdgeAll));
    addNumber(style, "flex", YGNodeStyleGetFlex(node));
    addNumber(style, "flexGrow", YGNodeStyleGetFlexGrow(node));
    if (YGNodeStyleGetFlexDirection(node) == YGFlexDirectionRow) {
        style += "flexDirection: \"row\", ";
    }
    if (!style.empty()) {
        style.resize(style.size() - 2);
    }

    const auto indent = std::string(static_cast<size_t>(depth) * 2, ' ');
    const auto count  = YGNodeGetChildCount(node);
    if (count == 0) {
        out += std::format("{}<Node style={{{{{}}}}} />\n", indent, style);
        return;
    }
    out += std::format("{}<Node style={{{{{}}}}}>\n", indent, style);
    for (size_t i = 0; i < count; i++) {
        writeNode(YGNodeGetChild(node, i), depth + 1, out);
    }
    out += std::format("{}</Node>\n", indent);
}
}  // namespace

namespace game::ui {

std::string yogaToPlayground(const YGNodeRef root) {
    std::string out = "<Layout config={{useWebDefaults: false}}>\n";
    writeNode(root, 1, out);
    out += "</Layout>\n";
    return out;
}

}  // namespace game::ui
