#pragma once

#include <yoga/Yoga.h>
#include <string_view>

namespace game::ui {

class YogaConfig {
public:
    static void applyConfig(YGNodeRef node, std::string_view config);

    static void applyStyle(YGNodeRef node, std::string_view style);

private:
    static void applyProperty(YGNodeRef node, std::string_view key,
                              std::string_view value);
};

}  // namespace game::ui
