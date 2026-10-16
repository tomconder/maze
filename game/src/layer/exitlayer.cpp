#include "layer/exitlayer.hpp"

#include "maze.hpp"
#include "ui/keyhints.hpp"

#include <glm/glm.hpp>

#include <array>
#include <optional>
#include <string_view>

namespace {
using game::layer::ExitMenuItem;

constexpr size_t menuItemCount = static_cast<size_t>(ExitMenuItem::Count);

// Buttons follow ExitMenuItem order, so the enum is the button index.
constexpr std::array<std::string_view, menuItemCount> menuLabels = {
    "Continue",
    "Options",
    "Return to Menu",
    "Exit the Game",
};

game::layer::ButtonMenuStyle exitStyle() {
    return {
        .name            = "exit",
        .labels          = menuLabels,
        .keyHints        = game::layer::exitKeyHints,
        .layout          = game::layer::exitMenuLayout,
        .buttonColor     = { 0.05F, 0.05F, 0.05F, 1.F },
        .selectedColor   = { 0.84F, 0.04F, 0.04F, 0.07F },
        .backgroundColor = std::nullopt,
        .modal           = true,
    };
}
}  // namespace

namespace game::layer {

ExitLayer::ExitLayer() : ButtonMenuLayer(exitStyle()) {}

bool ExitLayer::onBack() {
    activate(static_cast<size_t>(ExitMenuItem::Continue));
    return true;
}

void ExitLayer::onSelect(const size_t item) {
    switch (static_cast<ExitMenuItem>(item)) {
        case ExitMenuItem::Continue:
            clearHoveredItems();
            resumeGame();
            break;
        case ExitMenuItem::Options:
            clearHoveredItems();
            Maze::get().getOptionLayer()->setActive(true);
            break;
        case ExitMenuItem::ReturnToMenu:
            clearHoveredItems();
            Maze::get().getIntroLayer()->setActive(true);
            Maze::get().getMazeLayer()->setActive(false);
            setActive(false);
            break;
        case ExitMenuItem::Exit:
            Maze::get().exit();
            break;
        default:
            break;
    }
}

void ExitLayer::resumeGame() {
    setActive(false);
    if (Maze::get().getMazeLayer()->isImguiActive()) {
        Maze::get().getImGuiLayer()->setActive(true);
    }
}
}  // namespace game::layer
