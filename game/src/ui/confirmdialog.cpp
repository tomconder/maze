#include "ui/confirmdialog.hpp"

#include "input/gameaction.hpp"
#include "ui/keyhints.hpp"

#include <fmt/format.h>

#include <cmath>
#include <optional>
#include <utility>

namespace game::ui {
namespace {
constexpr glm::vec4 dimColor        = { 0.F, 0.F, 0.F, 0.7F };
constexpr glm::vec4 panelColor      = { 0.05F, 0.05F, 0.05F, 1.F };
constexpr glm::vec4 panelBorder     = { 0.84F, 0.84F, 0.84F, 0.5F };
constexpr glm::vec4 buttonBorder    = { 1.F, 1.F, 1.F, 0.25F };
constexpr glm::vec4 selectedColor   = { 0.84F, 0.04F, 0.04F, 0.14F };
constexpr glm::vec3 textColor       = { 1.F, 1.F, 1.F };
constexpr glm::vec3 countdownColor  = { 0.6F, 0.6F, 0.6F };
constexpr float     cornerRadius    = 12.F;
constexpr float     panelBorderSize = 2.F;
}  // namespace

ConfirmDialog::ConfirmDialog(
    std::shared_ptr<sponge::platform::opengl::scene::BitmapFont> font,
    const uint32_t                                               fontSize) :
    font(std::move(font)),
    fontSize(fontSize),
    layout(std::make_unique<ConfirmLayout>()),
    quad(std::make_unique<sponge::platform::opengl::scene::Quad>()),
    acceptButton(makeMenuButton(acceptLabel, fontSize, this->font,
                                glm::vec4{ 0.F }, textColor)),
    cancelButton(makeMenuButton(cancelLabel, fontSize, this->font,
                                glm::vec4{ 0.F }, textColor)) {
    acceptButton->attach(layout->acceptNode);
    cancelButton->attach(layout->cancelNode);
}

void ConfirmDialog::open(const std::string_view newTitle,
                         const std::string_view newMessage, const float seconds,
                         const bool defaultAccept) {
    title        = newTitle;
    message      = newMessage;
    opened       = true;
    acceptChosen = defaultAccept;
    timed        = seconds > 0.F;
    remaining    = seconds;
    acceptButton->setHover(false);
    cancelButton->setHover(false);
}

void ConfirmDialog::setFontSize(const uint32_t size) {
    fontSize = size;
    acceptButton->setFontSize(size);
    cancelButton->setFontSize(size);
}

ConfirmResult
    ConfirmDialog::onInput(sponge::platform::glfw::core::InputManager& mgr) {
    using sponge::input::GameAction;
    const auto& input = mgr.getSnapshot();

    const bool left  = input.isActive(GameAction::MenuLeft);
    const bool right = input.isActive(GameAction::MenuRight);
    if (left || right) {
        const bool next = left && !right;
        mgr.consumeActive(GameAction::MenuLeft);
        mgr.consumeActive(GameAction::MenuRight);
        if (next != acceptChosen) {
            acceptChosen = next;
            playHoverClick();
        }
    }
    if (input.isActive(GameAction::MenuBack)) {
        mgr.consumeActive(GameAction::MenuBack);
        return ConfirmResult::Cancelled;
    }
    if (input.isActive(GameAction::MenuConfirm)) {
        mgr.consumeActive(GameAction::MenuConfirm);
        return acceptChosen ? ConfirmResult::Accepted :
                              ConfirmResult::Cancelled;
    }
    return ConfirmResult::Pending;
}

ConfirmResult ConfirmDialog::onMousePressed(const glm::vec2& position) {
    if (acceptButton->isInside(position)) {
        return ConfirmResult::Accepted;
    }
    if (cancelButton->isInside(position)) {
        return ConfirmResult::Cancelled;
    }
    return ConfirmResult::Pending;
}

void ConfirmDialog::onMouseMoved(const glm::vec2& position) {
    updateButtonHover(acceptButton.get(), position);
    updateButtonHover(cancelButton.get(), position);
}

ConfirmResult ConfirmDialog::onUpdate(const double elapsedTime,
                                      const float width, const float height) {
    if (timed) {
        remaining -= static_cast<float>(elapsedTime);
        if (remaining <= 0.F) {
            return ConfirmResult::Cancelled;
        }
    }

    const auto measure = fontMeasure(font);
    const auto countdownText =
        fmt::format("Reverting in {}", static_cast<int>(std::ceil(remaining)));
    layout->calculate(width, height, { measure, fontSize }, title, message,
                      timed ? std::optional<std::string_view>{ countdownText } :
                              std::nullopt);

    const auto panel = nodeRect(layout->panelNode);
    quad->render({ 0.F, 0.F }, { width, height }, dimColor);
    quad->render({ panel.x, panel.y },
                 { panel.x + panel.width, panel.y + panel.height }, panelColor,
                 cornerRadius, panelBorderSize, panelBorder);

    font->beginPass(fontSize);
    const auto titleRect   = nodeRect(layout->titleNode);
    const auto messageRect = nodeRect(layout->messageNode);
    font->render(title, { titleRect.x, titleRect.y }, textColor);
    font->render(message, { messageRect.x, messageRect.y }, textColor);
    if (timed) {
        const auto countdownRect = nodeRect(layout->countdownNode);
        font->render(countdownText, { countdownRect.x, countdownRect.y },
                     countdownColor);
    }
    font->endPass();

    for (const auto& [button, selected] :
         { std::pair{ acceptButton.get(), acceptChosen },
           std::pair{ cancelButton.get(), !acceptChosen } }) {
        updateMenuButtonVisuals(button, selected, selectedColor);
        if (!selected && !button->hasHover()) {
            button->setBorderWidth(1.F);
            button->setBorderColor(buttonBorder);
        }
        button->onUpdate(elapsedTime);
    }

    return ConfirmResult::Pending;
}

}  // namespace game::ui
