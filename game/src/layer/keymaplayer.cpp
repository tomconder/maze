#include "layer/keymaplayer.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

#include "input/gameaction.hpp"
#include "maze.hpp"
#include "platform/glfw/core/application.hpp"
#include "platform/glfw/core/inputmanager.hpp"
#include "ui/keyhints.hpp"
#include "ui/rowparts.hpp"

namespace {
constexpr std::string_view captureMessage = "Press a key";

constexpr glm::vec3 textColor    = { 1.F, 1.F, 1.F };
constexpr glm::vec3 captureColor = { 1.F, 0.84F, 0.2F };

constexpr float textMarginLeft = 26.F;

// The row is the label, then the value. The value part has no width: it marks
// the right edge, and the text hangs left of it.
constexpr int valuePart = 1;

using game::layer::KeyMapItem;

constexpr size_t rowCount = static_cast<size_t>(KeyMapItem::Count);

struct BindingRow {
    sponge::input::GameAction action;
    std::string_view          label;
};

// Rows follow KeyMapItem order, so the enum is the row index. Menu navigation
// is absent on purpose: rebinding it could leave no way out of this screen.
constexpr std::array<BindingRow, rowCount> bindingRows = {
    BindingRow{ sponge::input::GameAction::MoveForward, "Move Forward" },
    BindingRow{ sponge::input::GameAction::MoveBack, "Move Back" },
    BindingRow{ sponge::input::GameAction::MoveLeft, "Move Left" },
    BindingRow{ sponge::input::GameAction::MoveRight, "Move Right" },
    BindingRow{ sponge::input::GameAction::Pause, "Pause" },
    BindingRow{ sponge::input::GameAction::ToggleDebugUI, "Debug UI" },
};

constexpr size_t resetHint = 3;  // Reset to Defaults in keyMapKeyHints

game::layer::ScrollMenuStyle keyMapStyle() {
    return {
        .name       = "keymap",
        .tab        = game::ui::OptionTab::Keyboard,
        .rowCount   = rowCount,
        .keyHints   = game::layer::keyMapKeyHints,
        .actionHint = resetHint,
    };
}

sponge::platform::glfw::core::InputManager& inputManager() {
    return sponge::platform::glfw::core::Application::get().getInputManager();
}
}  // namespace

namespace game::layer {

KeyMapLayer::KeyMapLayer() : ScrollMenuLayer(keyMapStyle()) {}

void KeyMapLayer::onAttach() {
    ScrollMenuLayer::onAttach();
    for (size_t row = 0; row < rowCount; row++) {
        ui::attachRowParts(rowNode(row), 2, textMarginLeft);
    }
    onFontSizeChanged();
    recalculateLayout();
}

void KeyMapLayer::onFontSizeChanged() {
    const auto textHeight =
        static_cast<float>(menuFont()->getHeight(menuFontSize()));
    for (size_t row = 0; row < rowCount; row++) {
        ui::setPartSize(rowNode(row), ui::labelPart, YGUndefined, textHeight);
        ui::setPartSize(rowNode(row), valuePart, 0.F, textHeight);
    }
}

bool KeyMapLayer::onUpdate(const double elapsedTime) {
    // The capture frame reports no actions at all, so nothing in the base fires
    // on the key being bound.
    if (rebindingRow && !inputManager().isRebinding()) {
        rebindingRow.reset();
    }
    return ScrollMenuLayer::onUpdate(elapsedTime);
}

bool KeyMapLayer::takesInput() const {
    return !rebindingRow && !Maze::get().getConfirmLayer()->isActive();
}

void KeyMapLayer::renderRow(const size_t row, const float /*x*/, const float y,
                            const float /*w*/, const float /*h*/) {
    const auto& labelFont = menuFont();
    const auto  labelSize = menuFontSize();

    const bool capturing = rebindingRow == row;
    const auto value =
        capturing ? std::string(captureMessage) :
                    sponge::platform::glfw::core::InputManager::keyLabel(
                        inputManager().getPrimaryKey(bindingRows[row].action));
    const auto valueWidth =
        static_cast<float>(labelFont->getLength(value, labelSize));
    const auto labelRect = ui::partRect(rowNode(row), ui::labelPart, y);
    const auto valueRect = ui::partRect(rowNode(row), valuePart, y);

    labelFont->beginPass(labelSize);
    labelFont->render(bindingRows[row].label, { labelRect.x, labelRect.y },
                      textColor);
    labelFont->render(value, { valueRect.x - valueWidth, valueRect.y },
                      capturing ? captureColor : textColor);
    labelFont->endPass();
}

void KeyMapLayer::onRowKeys(const size_t /*row*/,
                            const sponge::input::InputSnapshot& input) {
    using sponge::input::GameAction;
    if (input.isActive(GameAction::MenuReset)) {
        inputManager().consumeActive(GameAction::MenuReset);
        askReset();
    }
}

bool KeyMapLayer::onConfirm(const size_t row) {
    startRebind(row);
    return true;
}

void KeyMapLayer::onActionHint() {
    askReset();
}

bool KeyMapLayer::onRowPressed(const size_t row, const glm::vec2& /*mouse*/) {
    startRebind(row);
    return false;
}

void KeyMapLayer::startRebind(const size_t row) {
    rebindingRow = row;
    inputManager().requestRebind(bindingRows[row].action);
}

void KeyMapLayer::askReset() {
    clearHoveredItems();
    Maze::get().getConfirmLayer()->show(
        resetBindingsTitle, resetBindingsMessage,
        [](const ui::ConfirmResult result) {
            if (result == ui::ConfirmResult::Accepted) {
                inputManager().requestResetBindings();
            }
        },
        0.F, false);
}
}  // namespace game::layer
