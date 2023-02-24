#pragma once

#include "base.h"

namespace Sponge {

using MouseCode = uint16_t;

// inspired by glfw3.h
enum : MouseCode {
    Button0 = 0,
    Button1 = 1,
    Button2 = 2,
    Button3 = 3,
    Button4 = 4,
    Button5 = 5,
    Button6 = 6,
    Button7 = 7,
    Button8 = 8,

    ButtonLast = Button7,
    ButtonLeft = Button0,
    ButtonRight = Button1,
    ButtonMiddle = Button2
};

}  // namespace Sponge
