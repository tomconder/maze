#include "ui/keyhints.hpp"

#include "input/activedevice.hpp"
#include "input/inputsnapshot.hpp"
#include "platform/glfw/core/application.hpp"
#include "platform/glfw/core/inputmanager.hpp"
#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/scene/bitmapfont.hpp"
#include "platform/opengl/scene/sprite.hpp"
#include "platform/opengl/scene/spriteatlas.hpp"
#include "ui/menufontsize.hpp"

#include <glm/glm.hpp>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {
constexpr glm::vec3 labelColor = { 0.9F, 0.9F, 0.9F };
constexpr glm::vec3 hoverColor = { 1.F, 0.45F, 0.45F };

struct Metrics {
    uint32_t size;
    float    iconSize;
    float    gap;
    float    marginX;
    float    marginY;
};

Metrics metricsFor(const float windowWidth) {
    // one step below the menu size, and only sizes the atlas bakes
    // (18, 24, 32, 48) — an unbaked size renders no glyphs
    const auto menuSize =
        game::ui::menuFontSizeForWidth(static_cast<uint32_t>(windowWidth));
    const uint32_t size = menuSize <= 24 ? 18 : menuSize <= 32 ? 24 : 32;

    // the prompt art is inset in its sprite, so it needs more room than the
    // label text to read at the same weight
    const auto iconSize = static_cast<float>(size) * 2.5F;

    return { size, iconSize, static_cast<float>(size) * 0.5F,
             windowWidth * 0.03F, iconSize * 0.3F };
}

// Single-key prompts fill 48 px of their 64 px sprite, the arrow clusters only
// 30 px, so single keys draw smaller to match the arrows.
float iconScale(const std::string_view icon) {
    return icon == "keyboard_enter" || icon == "keyboard_escape" ||
                   icon == "keyboard_r" ?
               30.F / 48.F :
               1.F;
}

bool usingGamepad() {
    return sponge::platform::glfw::core::Application::get()
               .getInputManager()
               .getSnapshot()
               .activeDevice == sponge::input::ActiveDevice::Gamepad;
}

std::string_view iconOf(const game::ui::KeyHint& hint, const bool gamepad) {
    return gamepad ? hint.padIcon : hint.keyIcon;
}

using FontPtr = std::shared_ptr<sponge::platform::opengl::scene::BitmapFont>;

float hintWidth(const game::ui::KeyHint& hint, const FontPtr& font,
                const Metrics& metrics, const bool gamepad) {
    return metrics.iconSize * iconScale(iconOf(hint, gamepad)) + metrics.gap +
           static_cast<float>(font->getLength(hint.label, metrics.size));
}

bool isBack(const game::ui::KeyHint& hint) {
    return hint.keyIcon == "keyboard_escape";
}

// Back and the hints marked leftAligned sit at the left margin.
bool isLeft(const game::ui::KeyHint& hint) {
    return isBack(hint) || hint.leftAligned;
}

// Left x of every hint. The left group runs from the left margin, Back first
// so it sits in the same place on every menu; the rest are right-aligned
// against the right margin, in order. A window too narrow for both keeps the
// left group whole and lets the right group run past the margin.
std::vector<float> placeHints(std::span<const game::ui::KeyHint> hints,
                              const FontPtr& font, const Metrics& metrics,
                              const float windowWidth, const bool gamepad) {
    std::vector<float> xs(hints.size());
    const auto         step = metrics.gap * 2.F;

    auto leftEnd = metrics.marginX;
    for (const bool backPass : { true, false }) {
        for (size_t i = 0; i < hints.size(); i++) {
            if (isLeft(hints[i]) && isBack(hints[i]) == backPass) {
                xs[i] = leftEnd;
                leftEnd += hintWidth(hints[i], font, metrics, gamepad) + step;
            }
        }
    }

    float rightWidth = 0.F;
    for (const auto& hint : hints) {
        if (!isLeft(hint)) {
            rightWidth += hintWidth(hint, font, metrics, gamepad) + step;
        }
    }
    rightWidth = std::max(0.F, rightWidth - step);

    auto x = std::max(windowWidth - metrics.marginX - rightWidth,
                      leftEnd + metrics.gap * 2.F);
    for (size_t i = 0; i < hints.size(); i++) {
        if (!isLeft(hints[i])) {
            xs[i] = x;
            x += hintWidth(hints[i], font, metrics, gamepad) + step;
        }
    }
    return xs;
}

}  // namespace

namespace game::ui {
using sponge::platform::opengl::scene::BitmapFont;

const sponge::platform::opengl::scene::Sprite&
    promptSprite(const std::string_view name) {
    // The baked UI sprite sheet: the input prompt icons. Built on first use,
    // which needs a current GL context.
    static const sponge::platform::opengl::scene::SpriteAtlas atlas{
        "ui", "textures/ui.ktx2"
    };
    return atlas.sprite(name);
}

float keyHintBarHeight(const float windowWidth) {
    const auto metrics = metricsFor(windowWidth);
    return metrics.iconSize + metrics.marginY * 2.F;
}

float heightWithoutKeyHints(const float windowWidth, const float windowHeight) {
    return std::max(0.F, windowHeight - keyHintBarHeight(windowWidth));
}

void renderKeyHints(std::span<const KeyHint>           hints,
                    const std::shared_ptr<BitmapFont>& font,
                    const glm::mat4& projection, const float windowWidth,
                    const float                 windowHeight,
                    const std::optional<size_t> hovered) {
    if (hints.empty()) {
        return;
    }

    const auto metrics = metricsFor(windowWidth);
    const auto [size, iconSize, gap, marginX, marginY] = metrics;

    const auto iconTop = windowHeight - marginY - iconSize;
    const auto textTop =
        iconTop + (iconSize - static_cast<float>(font->getHeight(size))) / 2.F;

    const bool gamepad = usingGamepad();
    const auto xs      = placeHints(hints, font, metrics, windowWidth, gamepad);

    // every prompt shares the one sprite shader
    const auto& shader = promptSprite(hints.front().keyIcon).getShader();
    shader->setMat4("projection", projection);

    for (size_t i = 0; i < hints.size(); i++) {
        const auto icon      = iconOf(hints[i], gamepad);
        const auto drawnSize = iconSize * iconScale(icon);

        promptSprite(icon).render(
            { xs[i], iconTop + ((iconSize - drawnSize) / 2.F) },
            { drawnSize, drawnSize }, 1.F);

        font->beginPass(size);
        font->render(hints[i].label, { xs[i] + drawnSize + gap, textTop },
                     hovered == i ? hoverColor : labelColor);
        font->endPass();
    }
}

std::optional<size_t> hitTestKeyHints(std::span<const KeyHint>           hints,
                                      const std::shared_ptr<BitmapFont>& font,
                                      const float      windowWidth,
                                      const float      windowHeight,
                                      const glm::vec2& position) {
    const auto metrics = metricsFor(windowWidth);
    const auto iconTop = windowHeight - metrics.marginY - metrics.iconSize;
    if (position.y < iconTop || position.y > iconTop + metrics.iconSize) {
        return std::nullopt;
    }

    const bool gamepad = usingGamepad();
    const auto xs      = placeHints(hints, font, metrics, windowWidth, gamepad);
    for (size_t i = 0; i < hints.size(); i++) {
        const auto width = hintWidth(hints[i], font, metrics, gamepad);
        if (position.x >= xs[i] && position.x <= xs[i] + width) {
            return i;
        }
    }
    return std::nullopt;
}
}  // namespace game::ui
