#include "layer/introlayer.hpp"

#include "maze.hpp"
#include "ui/keyhints.hpp"

#include <glm/glm.hpp>

#include <array>
#include <string_view>

namespace {
using game::layer::IntroMenuItem;

constexpr size_t menuItemCount = static_cast<size_t>(IntroMenuItem::Count);

// Buttons follow IntroMenuItem order, so the enum is the button index.
constexpr std::array<std::string_view, menuItemCount> menuLabels = {
    "New Game",
    "Options",
    "Quit",
};

constexpr std::array<game::ui::KeyHint, 2> introKeyHints = {
    game::ui::KeyHint{ "keyboard_arrows_vertical", "xbox_dpad_vertical",
                       "Navigate" },
    game::ui::KeyHint{ "keyboard_enter", "xbox_button_a", "Select" },
};

game::layer::ButtonMenuStyle introStyle() {
    return {
        .name            = "intro",
        .labels          = menuLabels,
        .keyHints        = introKeyHints,
        .layout          = game::layer::introMenuLayout,
        .buttonColor     = { 0.F, 0.F, 0.F, 0.F },
        .selectedColor   = { 0.84F, 0.04F, 0.04F, 0.07F },
        .backgroundColor = glm::vec4{ 0.12F, 0.19F, 0.29F, 1.F },
    };
}
}  // namespace

namespace game::layer {

IntroLayer::IntroLayer() : ButtonMenuLayer(introStyle()) {}

void IntroLayer::beginFadeIn(const double duration) {
    isFadingIn        = true;
    fadeInDuration    = duration;
    fadeInAccumulator = 0.0;
    setActive(true);
}

bool IntroLayer::takesInput() const {
    return !isFadingIn && ButtonMenuLayer::takesInput();
}

bool IntroLayer::onUpdate(const double elapsedTime) {
    const bool running = ButtonMenuLayer::onUpdate(elapsedTime);

    if (isFadingIn && !Maze::get().isOptionsOpen()) {
        fadeInAccumulator += elapsedTime;
        if (fadeInAccumulator >= fadeInDuration) {
            isFadingIn = false;
        }
    }
    return running;
}

void IntroLayer::onSelect(const size_t item) {
    switch (static_cast<IntroMenuItem>(item)) {
        case IntroMenuItem::NewGame:
            clearHoveredItems();
            setActive(false);
            if (Maze::get().getMazeLayer()->isLoaded()) {
                Maze::get().getMazeLayer()->activate();
            } else {
                Maze::get().getLoadingLayer()->setActive(true);
            }
            break;
        case IntroMenuItem::Options:
            clearHoveredItems();
            Maze::get().getOptionLayer()->setActive(true);
            break;
        case IntroMenuItem::Quit:
            Maze::get().exit();
            break;
        default:
            break;
    }
}
}  // namespace game::layer
