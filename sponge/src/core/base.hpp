#pragma once

#include <type_traits>

#define BIT(x)    (1 << (x))
#define UNUSED(x) (void)(x)

// Unary + converts a scoped enum to its underlying integer,
// e.g. array[+GameAction::Jump] instead of a static_cast.
template <typename Enum>
constexpr auto operator+(Enum e) noexcept -> std::underlying_type_t<Enum> {
    return static_cast<std::underlying_type_t<Enum>>(e);
}

namespace sponge::core {

void startupCore();
void shutdownCore();

}  // namespace sponge::core
