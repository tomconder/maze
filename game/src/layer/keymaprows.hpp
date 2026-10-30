#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "input/gameaction.hpp"
#include "ui/keyhints.hpp"
#include "ui/menulayout.hpp"
#include "ui/rowlayout.hpp"

namespace game::layer {

enum class KeyMapItem : uint8_t {
    MoveForward = 0,
    MoveBack,
    MoveLeft,
    MoveRight,
    Pause,
    ToggleDebugUI,
    ToggleFps,
    Count,
};

inline constexpr size_t keyMapRowCount = static_cast<size_t>(KeyMapItem::Count);

inline constexpr std::array<ui::KeyHint, 4> keyMapKeyHints = {
    ui::KeyHint{ "keyboard_arrows_vertical", "xbox_dpad_vertical", "Navigate" },
    ui::KeyHint{ "keyboard_enter", "xbox_button_a", "Rebind" },
    ui::KeyHint{ "keyboard_escape", "xbox_button_b", "Back" },
    ui::KeyHint{ "keyboard_r", "xbox_button_y", "Reset to Defaults", true },
};

struct BindingRow {
    sponge::input::GameAction action;
    std::string_view          label;
};

// Rows follow KeyMapItem order, so the enum is the row index. Menu navigation
// is absent on purpose: rebinding it could leave no way out of this screen.
inline constexpr std::array<BindingRow, keyMapRowCount> bindingRows = {
    BindingRow{ sponge::input::GameAction::MoveForward, "Move Forward" },
    BindingRow{ sponge::input::GameAction::MoveBack, "Move Back" },
    BindingRow{ sponge::input::GameAction::MoveLeft, "Move Left" },
    BindingRow{ sponge::input::GameAction::MoveRight, "Move Right" },
    BindingRow{ sponge::input::GameAction::Pause, "Pause" },
    BindingRow{ sponge::input::GameAction::ToggleDebugUI, "Debug UI" },
    BindingRow{ sponge::input::GameAction::ToggleFps, "FPS Counter" },
};

// Every row of the keyboard tab is a label and a value. Needs no window.
void layoutKeyMapRows(const ui::MenuLayout& layout, const ui::RowText& text);

}  // namespace game::layer
