// sponge/src/input/inputsnapshot.cpp
#include "input/inputsnapshot.hpp"

namespace sponge::input {

std::string_view
    InputSnapshot::getPromptText(const GameAction a) const noexcept {
    if (activeDevice == ActiveDevice::Gamepad) {
        switch (a) {
            case GameAction::MenuUp:
                return "DPad Up";
            case GameAction::MenuDown:
                return "DPad Down";
            case GameAction::MenuLeft:
                return "DPad Left";
            case GameAction::MenuRight:
                return "DPad Right";
            case GameAction::MenuConfirm:
                return "A (Xbox)";
            case GameAction::MenuBack:
                return "B (Xbox)";
            case GameAction::MoveForward:
                return "Left Stick Up";
            case GameAction::MoveBack:
                return "Left Stick Down";
            case GameAction::MoveLeft:
                return "Left Stick Left";
            case GameAction::MoveRight:
                return "Left Stick Right";
            case GameAction::LookHorizontal:
                return "Right Stick X";
            case GameAction::LookVertical:
                return "Right Stick Y";
            case GameAction::Pause:
                return "Start";
            case GameAction::ExitGame:
                return "Back";
            case GameAction::ToggleFullscreen:
                return "";
            case GameAction::ToggleDebugUI:
                return "";
            default:
                return "";
        }
    }
    switch (a) {
        case GameAction::MenuUp:
            return "Up";
        case GameAction::MenuDown:
            return "Down";
        case GameAction::MenuLeft:
            return "Left";
        case GameAction::MenuRight:
            return "Right";
        case GameAction::MenuConfirm:
            return "Enter";
        case GameAction::MenuBack:
            return "Escape";
        case GameAction::MoveForward:
            return "W";
        case GameAction::MoveBack:
            return "S";
        case GameAction::MoveLeft:
            return "A";
        case GameAction::MoveRight:
            return "D";
        case GameAction::LookHorizontal:
            return "Mouse X";
        case GameAction::LookVertical:
            return "Mouse Y";
        case GameAction::Pause:
            return "Escape";
        case GameAction::ExitGame:
            return "";
        case GameAction::ToggleFullscreen:
            return "F";
        case GameAction::ToggleDebugUI:
            return "`";
        default:
            return "";
    }
}

}  // namespace sponge::input
