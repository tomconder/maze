#include "ui/tabbar.hpp"

#include <glm/glm.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

#include "input/activedevice.hpp"
#include "input/inputsnapshot.hpp"
#include "maze.hpp"
#include "platform/glfw/core/application.hpp"
#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/scene/bitmapfont.hpp"
#include "platform/opengl/scene/quad.hpp"
#include "platform/opengl/scene/sprite.hpp"
#include "ui/keyhints.hpp"
#include "ui/menufontsize.hpp"

namespace {
constexpr glm::vec3 activeColor   = { 1.F, 1.F, 1.F };
constexpr glm::vec3 inactiveColor = { 0.55F, 0.55F, 0.55F };
// same highlight the menu rows use for the selected row
constexpr glm::vec4 activeFillColor = { 0.84F, 0.04F, 0.04F, 0.14F };
constexpr glm::vec4 activeDotColor  = { 1.F, 1.F, 1.F, 1.F };

constexpr std::string_view prevKeyIcon = "keyboard_q";
constexpr std::string_view nextKeyIcon = "keyboard_e";
constexpr std::string_view prevPadIcon = "xbox_lb";
constexpr std::string_view nextPadIcon = "xbox_rb";

constexpr size_t tabCount = static_cast<size_t>(game::ui::OptionTab::Count);

struct Metrics {
    uint32_t size;
    float    iconSize;
    float    iconGap;  // icon to the label beside it
    float    tabGap;   // label to label
    float    marginY;
    float    padX;  // highlight inset around the active label
};

Metrics metricsFor(const float windowWidth) {
    const auto size =
        game::ui::menuFontSizeForWidth(static_cast<uint32_t>(windowWidth));
    const auto fSize = static_cast<float>(size);

    // the prompt art is inset in its sprite, so it needs more room than the
    // label text to read at the same weight
    return { size,         fSize * 2.F,  fSize * 2.4F,
             fSize * 2.4F, fSize * 0.4F, fSize * 0.4F };
}

// Highlight quad, built on first use — construction needs a GL context.
const sponge::platform::opengl::scene::Quad& highlightQuad() {
    static const auto quad =
        std::make_unique<sponge::platform::opengl::scene::Quad>();
    return *quad;
}

struct TabRect {
    float x;
    float width;
};

// Left edge and width of every tab label. left is the strip's left edge, where
// the first prompt icon sits, so the labels start one icon further in.
std::array<TabRect, tabCount> tabRects(
    const std::shared_ptr<sponge::platform::opengl::scene::BitmapFont>& font,
    const float windowWidth, const float left) {
    const auto metrics = metricsFor(windowWidth);

    std::array<TabRect, tabCount> rects{};
    float                         x = left + metrics.iconSize + metrics.iconGap;
    for (size_t i = 0; i < tabCount; i++) {
        rects[i].width = static_cast<float>(
            font->getLength(game::ui::optionTabLabels[i], metrics.size));
        rects[i].x = x;
        x += rects[i].width + metrics.tabGap;
    }
    return rects;
}
}  // namespace

namespace game::ui {
using sponge::platform::opengl::scene::BitmapFont;

float tabBarHeight(const float windowWidth) {
    // Font-free so layouts can reserve the space before the font is loaded.
    const auto metrics = metricsFor(windowWidth);
    return metrics.iconSize + metrics.marginY * 2.F;
}

void renderTabBar(const OptionTab                    active,
                  const std::shared_ptr<BitmapFont>& font,
                  const glm::mat4& projection, const float windowWidth,
                  const float left) {
    const auto metrics = metricsFor(windowWidth);
    const auto rects   = tabRects(font, windowWidth, left);

    const auto iconTop = metrics.marginY;
    const auto textTop =
        iconTop +
        (metrics.iconSize - static_cast<float>(font->getHeight(metrics.size))) /
            2.F;

    // the highlight goes down first so the label sits on top of it
    const auto& rect        = rects[static_cast<size_t>(active)];
    const auto  labelHeight = static_cast<float>(font->getHeight(metrics.size));
    const auto  dotSize     = static_cast<float>(metrics.size) * 0.25F;

    // the highlight makes room on the left for the marker dot
    highlightQuad().render(
        { rect.x - metrics.padX * 2.F - dotSize, textTop - metrics.marginY },
        { rect.x + rect.width + metrics.padX,
          textTop + labelHeight + metrics.marginY },
        activeFillColor);

    // a square with a half-width radius is a circle
    const auto dotLeft = rect.x - metrics.padX - dotSize;
    const auto dotTop  = textTop + (labelHeight - dotSize) / 2.F;
    highlightQuad().render({ dotLeft, dotTop },
                           { dotLeft + dotSize, dotTop + dotSize },
                           activeDotColor, dotSize / 2.F);

    font->beginPass(metrics.size);
    for (size_t i = 0; i < tabCount; i++) {
        const bool isActive = static_cast<size_t>(active) == i;
        font->render(optionTabLabels[i], { rects[i].x, textTop },
                     isActive ? activeColor : inactiveColor);
    }
    font->endPass();

    const bool gamepad =
        sponge::platform::glfw::core::Application::get()
            .getInputManager()
            .getSnapshot()
            .activeDevice == sponge::input::ActiveDevice::Gamepad;
    const auto prevIcon = gamepad ? prevPadIcon : prevKeyIcon;
    const auto nextIcon = gamepad ? nextPadIcon : nextKeyIcon;

    // every prompt shares the one sprite shader
    const auto& shader = promptSprite(prevIcon).getShader();
    shader->bind();
    shader->setMat4("projection", projection);
    shader->unbind();

    const auto size = glm::vec2{ metrics.iconSize, metrics.iconSize };
    promptSprite(prevIcon).render(
        { rects.front().x - metrics.iconGap - metrics.iconSize, iconTop }, size,
        1.F);
    promptSprite(nextIcon).render(
        { rects.back().x + rects.back().width + metrics.iconGap, iconTop },
        size, 1.F);
}

std::optional<OptionTab> tabBarHitTest(const std::shared_ptr<BitmapFont>& font,
                                       const float      windowWidth,
                                       const float      left,
                                       const glm::vec2& position) {
    if (position.y < 0.F || position.y > tabBarHeight(windowWidth)) {
        return std::nullopt;
    }

    const auto rects = tabRects(font, windowWidth, left);
    for (size_t i = 0; i < tabCount; i++) {
        if (position.x >= rects[i].x &&
            position.x <= rects[i].x + rects[i].width) {
            return static_cast<OptionTab>(i);
        }
    }
    return std::nullopt;
}

void showOptionTab(const OptionTab tab) {
    Maze::get().getOptionLayer()->setActive(tab == OptionTab::Display);
    Maze::get().getKeyMapLayer()->setActive(tab == OptionTab::Keyboard);
}
}  // namespace game::ui
