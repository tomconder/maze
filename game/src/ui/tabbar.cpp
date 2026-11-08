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

struct Metrics {
    uint32_t size;
    float    iconSize;
    float    gap;      // between the nodes of the bar
    float    marginY;  // above and below the icons
    float    padX;     // highlight inset around the active label
};

Metrics metricsFor(const float windowWidth) {
    const auto size =
        game::ui::menuFontSizeForWidth(static_cast<uint32_t>(windowWidth));
    const auto fSize = static_cast<float>(size);

    // the prompt art is inset in its sprite, so it needs more room than text
    return { size, fSize * 2.F, fSize * 2.4F, fSize * 0.4F, fSize * 0.8F };
}

// Highlight quad, built on first use — construction needs a GL context.
const sponge::platform::opengl::scene::Quad& highlightQuad() {
    static const auto quad =
        std::make_unique<sponge::platform::opengl::scene::Quad>();
    return *quad;
}

}  // namespace

namespace game::ui {
using sponge::platform::opengl::scene::BitmapFont;

float tabIconSize(const float windowWidth) {
    return metricsFor(windowWidth).iconSize;
}

float tabGap(const float windowWidth) {
    return metricsFor(windowWidth).gap;
}

void layoutTab(const YGNodeRef tab, const float windowWidth,
               const TextSize& label) {
    const auto metrics = metricsFor(windowWidth);

    if (YGNodeGetChildCount(tab) == 0) {
        YGNodeStyleSetFlexDirection(tab, YGFlexDirectionRow);
        YGNodeStyleSetAlignItems(tab, YGAlignCenter);
        YGNodeInsertChild(tab, YGNodeNew(), tabparts::dot);
        YGNodeInsertChild(tab, YGNodeNew(), tabparts::label);
    }

    // the box of the tab is the highlight of the active one
    YGNodeStyleSetPadding(tab, YGEdgeHorizontal, metrics.padX);
    YGNodeStyleSetPadding(tab, YGEdgeVertical, metrics.marginY);
    YGNodeStyleSetGap(tab, YGGutterColumn, metrics.padX);

    // a square, drawn as a circle
    const auto  dotSize = static_cast<float>(metrics.size) * 0.25F;
    auto* const dot     = YGNodeGetChild(tab, tabparts::dot);
    YGNodeStyleSetWidth(dot, dotSize);
    YGNodeStyleSetHeight(dot, dotSize);

    auto* const text = YGNodeGetChild(tab, tabparts::label);
    YGNodeStyleSetWidth(text, label.width);
    YGNodeStyleSetHeight(text, label.height);
}

std::array<TextSize, tabCount> tabLabelSizes(const float        windowWidth,
                                             const TextMeasure& measure) {
    const auto size = metricsFor(windowWidth).size;

    std::array<TextSize, tabCount> sizes{};
    for (size_t i = 0; i < tabCount; i++) {
        sizes[i] = measure(optionTabLabels[i], size);
    }
    return sizes;
}

float tabBarHeight(const float windowWidth) {
    // Font-free so layouts can reserve the space before the font is loaded.
    const auto metrics = metricsFor(windowWidth);
    return metrics.iconSize + metrics.marginY * 2.F;
}

void renderTabBar(const OptionTab                    active,
                  const std::shared_ptr<BitmapFont>& font,
                  const glm::mat4& projection, const float windowWidth,
                  const TabBarRects& rects) {
    const auto metrics = metricsFor(windowWidth);

    const auto  index     = static_cast<size_t>(active);
    const auto& highlight = rects.tabs[index];
    const auto& dot       = rects.dots[index];

    highlightQuad().render(
        { highlight.x, highlight.y },
        { highlight.x + highlight.width, highlight.y + highlight.height },
        activeFillColor);

    // a square with a half-width radius is a circle
    highlightQuad().render({ dot.x, dot.y },
                           { dot.x + dot.width, dot.y + dot.height },
                           activeDotColor, dot.width / 2.F);

    font->beginPass(metrics.size);
    for (size_t i = 0; i < tabCount; i++) {
        const bool isActive = static_cast<size_t>(active) == i;
        font->render(optionTabLabels[i],
                     { rects.labels[i].x, rects.labels[i].y },
                     isActive ? activeColor : inactiveColor);
    }
    font->endPass();

    const bool gamepad  = usingGamepad();
    const auto prevIcon = gamepad ? prevPadIcon : prevKeyIcon;
    const auto nextIcon = gamepad ? nextPadIcon : nextKeyIcon;

    // every prompt shares the one sprite shader
    const auto& shader = promptSprite(prevIcon).getShader();
    shader->setMat4("projection", projection);

    promptSprite(prevIcon).render({ rects.prev.x, rects.prev.y },
                                  { rects.prev.width, rects.prev.height }, 1.F);
    promptSprite(nextIcon).render({ rects.next.x, rects.next.y },
                                  { rects.next.width, rects.next.height }, 1.F);
}

std::optional<OptionTab> tabBarHitTest(const TabBarRects& rects,
                                       const float        windowWidth,
                                       const glm::vec2&   position) {
    if (position.y < 0.F || position.y > tabBarHeight(windowWidth)) {
        return std::nullopt;
    }

    for (size_t i = 0; i < tabCount; i++) {
        if (position.x >= rects.tabs[i].x &&
            position.x <= rects.tabs[i].x + rects.tabs[i].width) {
            return static_cast<OptionTab>(i);
        }
    }
    return std::nullopt;
}

void showOptionTab(const OptionTab tab) {
    Maze::get().getOptionLayer()->setActive(tab == OptionTab::Display);
    Maze::get().getKeyMapLayer()->setActive(tab == OptionTab::Keyboard);
    Maze::get().getAudioLayer()->setActive(tab == OptionTab::Audio);
}
}  // namespace game::ui
