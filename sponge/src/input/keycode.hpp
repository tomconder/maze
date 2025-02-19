#pragma once

#include <cstdint>
#include <ostream>

namespace sponge::input {

// inspired by glfw3.h
enum class KeyCode : uint16_t {
    SpongeKey_None = 0,

    // Printable keys
    SpongeKey_Space = 32,
    SpongeKey_Apostrophe = 39,
    SpongeKey_Comma = 44,
    SpongeKey_Minus = 45,
    SpongeKey_Period = 46,
    SpongeKey_Slash = 47,

    SpongeKey_D0 = 48,
    SpongeKey_D1 = 49,
    SpongeKey_D2 = 50,
    SpongeKey_D3 = 51,
    SpongeKey_D4 = 52,
    SpongeKey_D5 = 53,
    SpongeKey_D6 = 54,
    SpongeKey_D7 = 55,
    SpongeKey_D8 = 56,
    SpongeKey_D9 = 57,

    SpongeKey_Semicolon = 59,
    SpongeKey_Equal = 61,

    SpongeKey_A = 65,
    SpongeKey_B = 66,
    SpongeKey_C = 67,
    SpongeKey_D = 68,
    SpongeKey_E = 69,
    SpongeKey_F = 70,
    SpongeKey_G = 71,
    SpongeKey_H = 72,
    SpongeKey_I = 73,
    SpongeKey_J = 74,
    SpongeKey_K = 75,
    SpongeKey_L = 76,
    SpongeKey_M = 77,
    SpongeKey_N = 78,
    SpongeKey_O = 79,
    SpongeKey_P = 80,
    SpongeKey_Q = 81,
    SpongeKey_R = 82,
    SpongeKey_S = 83,
    SpongeKey_T = 84,
    SpongeKey_U = 85,
    SpongeKey_V = 86,
    SpongeKey_W = 87,
    SpongeKey_X = 88,
    SpongeKey_Y = 89,
    SpongeKey_Z = 90,

    SpongeKey_LeftBracket = 91,
    SpongeKey_Backslash = 92,
    SpongeKey_RightBracket = 93,
    SpongeKey_GraveAccent = 96,

    // Non-US keys
    SpongeKey_World1 = 161,
    SpongeKey_World2 = 162,

    // Function keys
    SpongeKey_Escape = 256,
    SpongeKey_Enter = 257,
    SpongeKey_Tab = 258,
    SpongeKey_Backspace = 259,
    SpongeKey_Insert = 260,
    SpongeKey_Delete = 261,
    SpongeKey_Right = 262,
    SpongeKey_Left = 263,
    SpongeKey_Down = 264,
    SpongeKey_Up = 265,
    SpongeKey_PageUp = 266,
    SpongeKey_PageDown = 267,
    SpongeKey_Home = 268,
    SpongeKey_End = 269,
    SpongeKey_CapsLock = 280,
    SpongeKey_ScrollLock = 281,
    SpongeKey_NumLock = 282,
    SpongeKey_PrintScreen = 283,
    SpongeKey_Pause = 284,
    SpongeKey_F1 = 290,
    SpongeKey_F2 = 291,
    SpongeKey_F3 = 292,
    SpongeKey_F4 = 293,
    SpongeKey_F5 = 294,
    SpongeKey_F6 = 295,
    SpongeKey_F7 = 296,
    SpongeKey_F8 = 297,
    SpongeKey_F9 = 298,
    SpongeKey_F10 = 299,
    SpongeKey_F11 = 300,
    SpongeKey_F12 = 301,
    SpongeKey_F13 = 302,
    SpongeKey_F14 = 303,
    SpongeKey_F15 = 304,
    SpongeKey_F16 = 305,
    SpongeKey_F17 = 306,
    SpongeKey_F18 = 307,
    SpongeKey_F19 = 308,
    SpongeKey_F20 = 309,
    SpongeKey_F21 = 310,
    SpongeKey_F22 = 311,
    SpongeKey_F23 = 312,
    SpongeKey_F24 = 313,

    // Keypad
    SpongeKey_KP0 = 320,
    SpongeKey_KP1 = 321,
    SpongeKey_KP2 = 322,
    SpongeKey_KP3 = 323,
    SpongeKey_KP4 = 324,
    SpongeKey_KP5 = 325,
    SpongeKey_KP6 = 326,
    SpongeKey_KP7 = 327,
    SpongeKey_KP8 = 328,
    SpongeKey_KP9 = 329,
    SpongeKey_KPDecimal = 330,
    SpongeKey_KPDivide = 331,
    SpongeKey_KPMultiply = 332,
    SpongeKey_KPSubtract = 333,
    SpongeKey_KPAdd = 334,
    SpongeKey_KPEnter = 335,
    SpongeKey_KPEqual = 336,

    SpongeKey_LeftShift = 340,
    SpongeKey_LeftControl = 341,
    SpongeKey_LeftAlt = 342,
    SpongeKey_LeftSuper = 343,
    SpongeKey_RightShift = 344,
    SpongeKey_RightControl = 345,
    SpongeKey_RightAlt = 346,
    SpongeKey_RightSuper = 347,
    SpongeKey_Menu = 348
};

inline std::ostream& operator<<(std::ostream& os, const KeyCode keyCode) {
    os << static_cast<int32_t>(keyCode);
    return os;
}

}  // namespace sponge::input
