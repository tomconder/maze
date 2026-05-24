#pragma once
#include <cstdint>

namespace sponge::input {

enum class InputContext : uint8_t { Gameplay = 0, Menu = 1 };

constexpr int operator+(const InputContext c) noexcept {
    return static_cast<int>(c);
}

}  // namespace sponge::input
