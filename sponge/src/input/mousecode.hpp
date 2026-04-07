#pragma once

#include <cstdint>
#include <ostream>

namespace sponge::input {

// inspired by glfw3.h
enum class MouseButton : uint8_t {
    Button_None = 0xFF,

    Button0 = 0,
    Button1 = 1,
    Button2 = 2,
    Button3 = 3,
    Button4 = 4,
    Button5 = 5,

    ButtonLeft   = Button0,
    ButtonRight  = Button1,
    ButtonMiddle = Button2
};

constexpr int operator+(const MouseButton button) noexcept {
    return static_cast<int>(button);
}

inline std::ostream& operator<<(std::ostream& os, const MouseButton button) {
    os << +button;
    return os;
}

}  // namespace sponge::input
