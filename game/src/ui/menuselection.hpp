#pragma once

#include "input/gameaction.hpp"
#include "input/inputsnapshot.hpp"

namespace game::ui {

// Moves the selection on MenuUp/MenuDown, wrapping at both ends. T is a menu
// enum with contiguous values and Count last.
template <typename T>
void stepSelection(const sponge::input::InputSnapshot& input, T& selected) {
    constexpr auto count = static_cast<int>(T::Count);

    if (input.isActive(sponge::input::GameAction::MenuDown)) {
        selected = static_cast<T>((static_cast<int>(selected) + 1) % count);
    }
    if (input.isActive(sponge::input::GameAction::MenuUp)) {
        selected =
            static_cast<T>((static_cast<int>(selected) - 1 + count) % count);
    }
}

}  // namespace game::ui
