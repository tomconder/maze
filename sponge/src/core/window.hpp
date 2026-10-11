#pragma once

#include <cstdint>
#include <string_view>

namespace sponge::core {

struct Resolution {
    uint32_t width;
    uint32_t height;
};

enum class DisplayMode : uint8_t {
    Windowed,
    FullScreen,
    Borderless,
    Count,
};

struct WindowProps {
    std::string_view title;
    uint32_t         width;
    uint32_t         height;
    DisplayMode      displayMode;
    bool             decorated = true;
};

}  // namespace sponge::core
