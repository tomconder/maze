#pragma once

#include <cstdint>
#include <ostream>

namespace sponge::input {

// inspired by glfw3.h
using Button = enum MouseButton : uint8_t {
    Button0 = 0,
    Button1 = 1,
    Button2 = 2,
    Button3 = 3,
    Button4 = 4,
    Button5 = 5,

    ButtonLeft = Button0,
    ButtonRight = Button1,
    ButtonMiddle = Button2
};

inline std::ostream& operator<<(std::ostream& os, const MouseButton button) {
    os << static_cast<int32_t>(button);
    return os;
}

}  // namespace sponge::input
