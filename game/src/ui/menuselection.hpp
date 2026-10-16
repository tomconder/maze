#pragma once

#include "input/gameaction.hpp"
#include "input/inputsnapshot.hpp"

#include <cstddef>

namespace game::ui {

// Moves the selection on MenuUp/MenuDown, wrapping at both ends. Returns true
// when changed.
inline bool stepSelection(const sponge::input::InputSnapshot& input,
                          size_t& selected, const size_t count) {
    bool changed = false;

    if (input.isActive(sponge::input::GameAction::MenuDown)) {
        selected = (selected + 1) % count;
        changed  = true;
    }
    if (input.isActive(sponge::input::GameAction::MenuUp)) {
        selected = (selected + count - 1) % count;
        changed  = true;
    }
    return changed;
}

}  // namespace game::ui
