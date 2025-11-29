#pragma once

#include <yoga/Yoga.h>
#include <string_view>

namespace game::ui {

// Parse and apply Yoga layout configuration from a string format
// inspired by yogalayout.dev playground syntax
class YogaConfig {
public:
    // Apply configuration string to a Yoga node
    // Config format example:
    // "flexDirection:column;justifyContent:center;alignItems:stretch"
    static void applyConfig(YGNodeRef node, const std::string_view config);

    // Apply style configuration (supports common Yoga properties)
    // Style format example:
    // "width:100%;height:100px;margin:10;padding:20;alignSelf:center"
    static void applyStyle(YGNodeRef node, const std::string_view style);

private:
    static void applyProperty(YGNodeRef node, const std::string_view key,
                              const std::string_view value);
};

}  // namespace game::ui
