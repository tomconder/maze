#pragma once

#include <cstdint>
#include <string_view>

namespace sponge::core {

struct Resolution {
    uint32_t width;
    uint32_t height;
};

struct WindowProps {
    std::string_view title;
    uint32_t         width;
    uint32_t         height;
    bool             fullscreen;
};

}  // namespace sponge::core
