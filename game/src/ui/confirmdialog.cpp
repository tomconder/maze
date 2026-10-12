#include "ui/confirmdialog.hpp"

#include "input/gameaction.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <cmath>
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
    const std::string_view title, const std::string_view message,
    std::shared_ptr<sponge::platform::opengl::scene::BitmapFont> font,
    const uint32_t                                               fontSize) :
    title(title),
    message(message),
    font(std::move(font)),
    fontSize(fontSize),
    quad(std::make_unique<sponge::platform::opengl::scene::Quad>()),
    acceptButton(makeMenuButton("Accept", fontSize, this->font,
                                glm::vec4{ 0.F }, textColor)),
    cancelButton(makeMenuButton("Cancel", fontSize, this->font,
                                glm::vec4{ 0.F }, textColor)) {
    acceptButton->setAlignType(ButtonAlignType::CenterAligned);
    cancelButton->setAlignType(ButtonAlignType::CenterAligned);
}

void ConfirmDialog::open(const float seconds) {
    opened       = true;
    acceptChosen = true;
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
    remaining -= static_cast<float>(elapsedTime);
    if (remaining <= 0.F) {
        return ConfirmResult::Cancelled;
    }

    const auto  size    = static_cast<float>(fontSize);
    const auto  lineH   = static_cast<float>(font->getHeight(fontSize));
    const float pad     = size * 1.25F;
    const float gap     = size * 0.5F;
    const float buttonH = size * 2.2F;

    const auto countdown =
        fmt::format("Reverting in {}", static_cast<int>(std::ceil(remaining)));
    const float textW  = static_cast<float>(std::max(
        font->getLength(title, fontSize), font->getLength(message, fontSize)));
    const float panelW = textW + pad * 2.F;
    const float panelH = pad * 2.F + lineH * 3.F + gap * 3.F + buttonH;
    const float panelX = std::round((width - panelW) / 2.F);
    const float panelY = std::round((height - panelH) / 2.F);

    quad->render({ 0.F, 0.F }, { width, height }, dimColor);
    quad->render({ panelX, panelY }, { panelX + panelW, panelY + panelH },
                 panelColor, cornerRadius, panelBorderSize, panelBorder);

    auto centered = [&](const std::string_view text, const float y,
                        const glm::vec3& color) {
        const auto len = static_cast<float>(font->getLength(text, fontSize));
        font->beginPass(fontSize);
        font->render(text, { std::round(panelX + (panelW - len) / 2.F), y },
                     color);
        font->endPass();
    };
    float y = panelY + pad;
    centered(title, y, textColor);
    y += lineH + gap;
    centered(message, y, textColor);
    y += lineH + gap;
    centered(countdown, y, countdownColor);
    y += lineH + gap;

    const float buttonW = (panelW - pad * 2.F - gap) / 2.F;
    const float leftX   = panelX + pad;
    const float rightX  = leftX + buttonW + gap;
    acceptButton->setPosition({ leftX, y }, { leftX + buttonW, y + buttonH });
    cancelButton->setPosition({ rightX, y }, { rightX + buttonW, y + buttonH });
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
